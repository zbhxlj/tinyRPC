#ifndef _SRC_RPC_PROTOCOL_MESSAGE_H_
#define _SRC_RPC_PROTOCOL_MESSAGE_H_

#include <cinttypes>

#include <memory>

#include "../../base/Enum.h"

namespace tinyRPC {

class Message {
 public:
  virtual ~Message() = default;

  enum class Type : std::uint64_t {
    // There is no stream involved.
    Single
  };

  // Value of this constant does not matter, as there's only one call on the
  // connection anyway.
  //
  // Note that 0 is not usable here as we use 0 as a guard value.
  //
  // FIXME: "Multiplex[a]ble" or "Multiplex[i]ble"? The former seems a bit more
  // common.
  static constexpr auto kNonmultiplexableCorrelationId = 1;

  // Correlation ID uniquely identifies a call.
  //
  // If multiplexing is no supported by the underlying protocol, return
  // `kNonmultiplexableCorrelationId`.
  virtual std::uint64_t GetCorrelationId() const noexcept = 0;

  // Returns type of this message. @sa: `Type`.
  virtual Type GetType() const noexcept = 0;
};

// Factory for producing "special" messages.
class MessageFactory {
 public:
  virtual ~MessageFactory() = default;

  enum class Type {
    // The framework asks the protocol object to create a message of this type
    // when it detects the server is overloaded. The resulting message will be
    // sent back to the caller as a response.
    //
    // Server side.
    Overloaded,

    // The framework creates a message of this type if it has detected too many
    // "overloaded" response from the callee. In this case, the framework
    // deliberately fails RPC requests made by the program for some time, to
    // prevent further pressure to the callee.
    //
    // Client side.
    CircuitBroken
  };

  // This method is permitted to "fail", i.e. returning `nullptr`. This won't
  // lead to a disastrous result. The caller is required to handle `nullptr`
  // gracefully (in most case this leads to a similar situation as "RPC
  // timeout".).
  virtual std::unique_ptr<Message> Create(Type type,
                                          std::uint64_t correlation_id,
                                          bool stream) const = 0;

  // A predefined factory that always returns `nullptr`.
  static const MessageFactory* null_factory;
};

TINYRPC_DEFINE_ENUM_BITMASK_OPS(Message::Type);

}  // namespace tinyRPC

#endif  
