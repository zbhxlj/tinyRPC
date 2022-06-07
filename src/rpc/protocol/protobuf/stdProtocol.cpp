#include "stdProtocol.h"
#include <cstring>

#include "../../../base/Endian.h"
#include "CallContext.h"
#include "CallContextFactory.h"
#include "Message.h"
#include "rpc_meta.pb.h"
#include "ServiceMethodLocator.h"

namespace tinyRPC::protobuf {

FLARE_RPC_REGISTER_CLIENT_SIDE_STREAM_PROTOCOL_ARG("flare", StdProtocol, false);
FLARE_RPC_REGISTER_SERVER_SIDE_STREAM_PROTOCOL_ARG("flare", StdProtocol, true);

namespace {

void RegisterMethodCallback(const google::protobuf::MethodDescriptor* method) {
  ServiceMethodLocator::Instance()->RegisterMethod(protocol_ids::standard,
                                                   method, method->full_name());
}

void DeregisterMethodCallback(
    const google::protobuf::MethodDescriptor* method) {
  ServiceMethodLocator::Instance()->DeregisterMethod(protocol_ids::standard,
                                                     method);
}

}  // namespace

FLARE_RPC_PROTOCOL_PROTOBUF_REGISTER_METHOD_PROVIDER(RegisterMethodCallback,
                                                     DeregisterMethodCallback);

namespace {

// All members are little-endian (after serialization).
struct Header {
  std::uint32_t magic;
  std::uint32_t meta_size;
  std::uint32_t msg_size;
  std::uint32_t att_size;
};

constexpr auto kHeaderSize = sizeof(Header);
const std::uint32_t kHeaderMagic = 'F' << 24 | 'R' << 16 | 'P' << 8 | 'C';

static_assert(kHeaderSize == 16);

struct OnWireMessage : public Message {
  OnWireMessage() {}
  std::uint64_t GetCorrelationId() const noexcept override {
    return meta->correlation_id();
  }
  Type GetType() const noexcept override {
    return FromWireType(meta->method_type(), meta->flags());
  }

  std::shared_ptr<rpc::RpcMeta> meta;
  std::string body;
  std::string attach;
};

StreamProtocol::Characteristics characteristics = {.name = "FlareStd"};

}  // namespace

const StreamProtocol::Characteristics& StdProtocol::GetCharacteristics() const {
  return characteristics;
}

const MessageFactory* StdProtocol::GetMessageFactory() const {
  return &error_message_factory;
}

const ControllerFactory* StdProtocol::GetControllerFactory() const {
  return &passive_call_context_factory;
}

StdProtocol::MessageCutStatus StdProtocol::TryCutMessage(
    std::string& buffer, std::unique_ptr<Message>* message) {
  if (buffer.size() < kHeaderSize) {
    return MessageCutStatus::NotIdentified;
  }

  // Extract the header (and convert the endianness if necessary) first.
  Header hdr;
  memcpy(&hdr, buffer.data(), kHeaderSize);
  FromLittleEndian(&hdr.magic);
  FromLittleEndian(&hdr.meta_size);
  FromLittleEndian(&hdr.msg_size);
  FromLittleEndian(&hdr.att_size);

  if (hdr.magic != kHeaderMagic) {
    return MessageCutStatus::ProtocolMismatch;
  }
  if (buffer.size() < static_cast<std::uint64_t>(kHeaderSize) +
                               hdr.meta_size + hdr.msg_size + hdr.att_size) {
    return MessageCutStatus::NeedMore;
  }

  // Do basic parse.
  std::string cut =
      buffer.substr(kHeaderSize, hdr.meta_size + hdr.msg_size + hdr.att_size);

  // Parse the meta.
  auto meta = std::make_shared<rpc::RpcMeta>();
  bool parsed = false;

  parsed = meta->ParseFromArray(cut.data(), hdr.meta_size);

  // We need to consume the body / attachment anyway otherwise we would leave
  // the buffer in non-packet-boundary.
  auto body_buffer = cut.substr(hdr.meta_size, hdr.msg_size);
  auto attach_buffer = cut.substr(hdr.meta_size + hdr.msg_size, hdr.att_size);

  // If parsing meta failed, raise an error now.
  if (!parsed) {
    FLARE_LOG_WARNING("Invalid meta received, dropped.");
    return MessageCutStatus::Error;
  }

  // We've cut the message then.
  auto msg = std::make_unique<OnWireMessage>();
  msg->meta = std::move(meta);
  msg->body = std::move(body_buffer);
  msg->attach = std::move(attach_buffer);
  *message = std::move(msg);
  return MessageCutStatus::Cut;
}

bool StdProtocol::TryParse(std::unique_ptr<Message>* message,
                           Controller* controller) {
  auto on_wire = static_cast<OnWireMessage*>(message->get());
  auto parsed = std::make_unique<ProtoMessage>();

  // Let's move them early.
  parsed->meta = std::move(on_wire->meta);

  auto&& meta = parsed->meta;

  if ((server_side_ && !meta->has_request_meta()) ||
      (!server_side_ && !meta->has_response_meta())) {
    FLARE_LOG_WARNING(
        "Corrupted message: Request or response meta is not present. "
        "Correlation ID {}.",
        meta->correlation_id());
    return false;
  }

  MaybeOwning<google::protobuf::Message> unpack_to;

  if (server_side_) {
    auto&& method = meta->request_meta().method_name();
    auto desc = ServiceMethodLocator::Instance()->TryGetMethodDesc(
        protocol_ids::standard, method);
    if (!desc) {
      // Instead of dropping the packet, we could produce an `EarlyErrorMessage`
      // of `kMethodNotFound` and let the upper layer return an error more
      // gracefully.
      FLARE_VLOG(1, "Method [{}] is not found.", method);
      *message = std::make_unique<EarlyErrorMessage>(
          meta->correlation_id(), rpc::STATUS_METHOD_NOT_FOUND,
          fmt::format("Method [{}] is not implemented.", method));
      return true;
    }

    unpack_to = std::unique_ptr<google::protobuf::Message>(
        desc->request_prototype->New());
  } else {
    FLARE_CHECK(meta->has_response_meta());  // Checked before.
    auto ctx = static_cast<ProactiveCallContext*>(controller);
    unpack_to = ctx->GetOrCreateResponse();
  }

  if (FLARE_LIKELY(!(meta->flags() & rpc::MESSAGE_FLAGS_NO_PAYLOAD))) {
    if (FLARE_LIKELY(unpack_to)) {
      if (!unpack_to->ParseFromString(on_wire->body)) {
        FLARE_LOG_WARNING(
            "Failed to parse message (correlation id {}).",
            meta->correlation_id());
        return false;
      }
      parsed->msg = std::move(unpack_to);
    } else {
      FLARE_CHECK(0, "Does not support raw bytes\n");
    }
  }

  if (!on_wire->attach.empty()) {
      parsed->attachment = std::move(on_wire->attach);
  }
  *message = std::move(parsed);
  return true;
}

// Serialize `message` into `buffer`.
void StdProtocol::WriteMessage(const Message& message,
                               std::string& buffer,
                               Controller* controller) {
  auto old_size = buffer.size();
  auto msg = static_cast<ProtoMessage*>(const_cast<Message*>(&message));
  auto meta = *msg->meta;  // Copied, likely to be slow.
  auto&& att = msg->attachment;

  std::string builder;

  Header hdr = {
      .magic = kHeaderMagic,
      .meta_size = static_cast<std::uint32_t>(meta.ByteSizeLong()),
      .msg_size = 0 /* Filled later. */,
      .att_size = 0 /* Filled later. */};

  {
    FLARE_CHECK(meta.SerializeToString(&builder));  // Meta.
  }

  // Body  
  hdr.msg_size += tinyRPC::protobuf::WriteTo(msg->msg, builder);

  // Attachment.
  if (!att.empty()) {
      builder += att;
      hdr.att_size += att.size();
  }

  // Fill header.
  ToLittleEndian(&hdr.magic);
  ToLittleEndian(&hdr.meta_size);
  ToLittleEndian(&hdr.msg_size);
  ToLittleEndian(&hdr.att_size);

  char headerBuffer [sizeof(Header)];
  memcpy(headerBuffer, &hdr, sizeof(Header));
  builder = std::string(headerBuffer) + builder;

  buffer += builder;
  FLARE_CHECK_EQ(buffer.size() - old_size,
                 kHeaderSize + hdr.meta_size + hdr.msg_size + hdr.att_size);
}

}  // namespace tinyRPC::protobuf
