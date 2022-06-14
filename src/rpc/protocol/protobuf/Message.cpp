#include "Message.h"

#include <string>

#include "gflags/gflags.h"

#include "../../../base/Enum.h"
#include "../../../base/Logging.h"
#include "../../../base/String.h"

namespace tinyRPC::protobuf {

namespace {

std::unique_ptr<Message> CreateErrorMessage(std::uint64_t correlation_id, int status,
                                            std::string desc) {
  auto meta = std::make_shared<rpc::RpcMeta>();
  meta->set_correlation_id(correlation_id);
  meta->set_method_type(rpc::METHOD_TYPE_SINGLE);
  auto&& resp_meta = *meta->mutable_response_meta();
  resp_meta.set_status(status);
  resp_meta.set_description(std::move(desc));
  return std::make_unique<ProtoMessage>(std::move(meta), nullptr);
}

}  // namespace

ErrorMessageFactory error_message_factory;

std::string Write(const PBMessage& msg) {
  std::string nbb;
  WriteTo(msg, nbb);
  return nbb;
}

std::size_t WriteTo(const PBMessage& msg,
                    std::string& builder) {
  if (!msg.has_value()) {
    return 0;
  } else{
    auto&& pb_msg = msg.value();
    if (FLARE_LIKELY(pb_msg)) {
      // `msg->InInitialized()` is not checked here, it's too slow to be checked
      // in optimized build. For non-optimized build, it's already checked by
      // default (by Protocol Buffers' generated code).
      FLARE_CHECK(pb_msg->SerializeToString(&builder));
      return pb_msg->ByteSizeLong();
    } else {
      return 0;
    }
  }
}

EarlyErrorMessage::EarlyErrorMessage(std::uint64_t correlation_id,
                                     rpc::Status status, std::string desc)
    : correlation_id_(correlation_id), status_(status), desc_(std::move(desc)) {
  FLARE_CHECK(status != rpc::STATUS_SUCCESS);  // Non-intended use.
}

std::uint64_t EarlyErrorMessage::GetCorrelationId() const noexcept {
  return correlation_id_;
}

Message::Type EarlyErrorMessage::GetType() const noexcept {
  return Type::Single;
}

rpc::Status EarlyErrorMessage::GetStatus() const { return status_; }

const std::string& EarlyErrorMessage::GetDescription() const { return desc_; }

std::unique_ptr<Message> ErrorMessageFactory::Create(
    Type type, std::uint64_t correlation_id) const {
  if (type == Type::Overloaded || type == Type::CircuitBroken) {
    return CreateErrorMessage(
        correlation_id,
        tinyRPC::rpc::STATUS_OVERLOADED,
        "Server overloaded.");
  }
  FLARE_LOG_WARNING(
      "Unknown message: type {}, correlation_id {}.",
      underlying_value(type), correlation_id);
  return nullptr;
}

Message::Type FromWireType(rpc::MethodType method_type, std::uint64_t flags) {
  if (method_type == rpc::METHOD_TYPE_SINGLE) {
    return Message::Type::Single;
  }
  FLARE_CHECK(0);
}

}  // namespace tinyRPC::protobuf
