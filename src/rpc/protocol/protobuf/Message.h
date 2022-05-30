#ifndef _SRC_RPC_PROTOCOL_PROTOBUF_MESSAGE_H_
#define _SRC_RPC_PROTOCOL_PROTOBUF_MESSAGE_H_

#include <limits>
#include <memory>
#include <string>
#include <optional>

#include "../../../base/Enum.h"
#include "../../../base/MaybeOwning.h"
#include "../Message.h"
#include "rpc_meta.pb.h"

namespace tinyRPC::protobuf {

// Either a:
//
// - Empty state: no payload is present.
// - A pointer to actual message.
// - Binary bytes: `accept_xxx_in_raw_bytes` is applied.
//
// TODO(luobogao): We need a dedicated C++ type for this (for cleaner code.).
using PBMessage =
    std::optional<MaybeOwning<const google::protobuf::Message>>;

// Serializes `PBMessage` to binary bytes.
std::string Write(const PBMessage& msg);

// Same as `Write`, but this one writes to an existing buffer builder.
//
// Number of bytes written is returned.
std::size_t WriteTo(const PBMessage& msg,
                    std::string& builder);

struct ProtoMessage : Message {
  ProtoMessage() {}
  ProtoMessage(std::shared_ptr<rpc::RpcMeta> meta, PBMessage&& msg,
               std::string attachment = {})
      : meta(std::move(meta)),
        msg(std::move(msg)),
        attachment(std::move(attachment)) {
  }

  std::uint64_t GetCorrelationId() const noexcept override {
    return meta->correlation_id();
  }
  Type GetType() const noexcept override;

  std::shared_ptr<rpc::RpcMeta> meta;
  PBMessage msg;
  std::string attachment;

};

// Recognized by `StreamService`. Used as a placeholder for notifying
// `StreamService` (and others) that some error occurred during parsing ("early
// stage") the message.
//
// Generating this message (via `ServerProtocol`) differs from returning error
// from protocol / service object in that by doing this, we'll be able to handle
// cases such as "method not found" more gracefully, without abruptly closing
// the connection.
class EarlyErrorMessage : public Message {
 public:
  explicit EarlyErrorMessage(std::uint64_t correlation_id, rpc::Status status,
                             std::string desc);

  std::uint64_t GetCorrelationId() const noexcept override;
  Type GetType() const noexcept override;

  rpc::Status GetStatus() const;
  const std::string& GetDescription() const;

 private:
  std::uint64_t correlation_id_;
  rpc::Status status_;
  std::string desc_;
};

// Factory for creating special messages.
class ErrorMessageFactory : public MessageFactory {
 public:
  std::unique_ptr<Message> Create(Type type, std::uint64_t correlation_id) const override;
};

extern ErrorMessageFactory error_message_factory;

Message::Type FromWireType(rpc::MethodType method_type, std::uint64_t flags);

inline Message::Type ProtoMessage::GetType() const noexcept {
  return FromWireType(meta->method_type(), meta->flags());
}

}  // namespace tinyRPC::protobuf

namespace tinyRPC {
TINYRPC_DEFINE_ENUM_BITMASK_OPS(rpc::MessageFlags);

}  // namespace tinyRPC

#endif
