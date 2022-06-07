#ifndef _SRC_RPC_PROTOCOL_STREAM_PROTOCOL_H_
#define _SRC_RPC_PROTOCOL_STREAM_PROTOCOL_H_

#include <memory>
#include <string>

#include "../../base/DependencyRegistry.h"
#include "../../base/Enum.h"
#include "Controller.h"
#include "Message.h"

namespace tinyRPC {

// Each instance of this class is bound to exactly one connection.
//
// Therefore, the implementation is permitted to cache whatever it sees
// suitable in its internal state about the connection it's processing.
class StreamProtocol {
 public:
  virtual ~StreamProtocol() = default;

  enum class MessageCutStatus : std::uint64_t {
    // More bytes needed for identifying data's protocol. This value is
    // generally returned when bytes given is not even large enough for the
    // header.
    NotIdentified,

    // The protocol itself is recognized, but more bytes are needed for cutting
    // off a single message.
    //
    // It's explicitly permitted to return this value even if `buffer` is
    // consumed. This also gives the implementation the ability to drop some
    // bytes without raise an error.
    NeedMore,

    // One message has been cut off successfully.
    Cut,

    // Unrecognized protocol.
    //
    // If the framework has not determined the protocol running on the
    // connection, the next protocol is tried. Otherwise this value is treated
    // as `Error`.
    //
    // Buffer gave to the protocol object must be left untouched.
    ProtocolMismatch,

    // Error occurred. Connection will be closed.
    Error,

  };

  struct Characteristics {
    std::string name;
    // TODO: useful?
    // If set, this protocol does not support multiplexing, and a dedicated
    // connection is required for each request (but the connection can be
    // reused.).
    //
    // In this case, all message produced / consumed by this protocol should
    // have a correlation_id of `kNonmultiplexableCorrelationId`.
    bool not_multiplexable = false;
  };

  // Arguably this should be a static method but it's impossible to mark a
  // virtual method as `static`.
  virtual const Characteristics& GetCharacteristics() const = 0;

  // This method returns factory for creating special messages that will be used
  // by the framework.
  virtual const MessageFactory* GetMessageFactory() const = 0;

  // This method returns factory for creating *server side* RPC controller to be
  // used with this protocol.
  //
  // If your protocol is only intended to be used at client side, you can return
  // `nullptr` (or `ControllerFactory::null_factory`, whichever fits your
  // taste.) here.
  virtual const ControllerFactory* GetControllerFactory() const = 0;

  // This method cuts a message out from `buffer`. The raw bytes corresponds to
  // message cut is returned in `msg_bytes`.
  //
  // The implementation is permitted to copy `buffer` to its internal state and
  // consume `buffer`. This may be required for handling protocols that
  // interleave messages.
  //
  // This method might be split into two methods in the future for better
  // flexibility: One for handling incoming bytes (and sending out replies if
  // needed) and save it internally, and one for cutting messages that are
  // recognized as completed out from its internal buffer. (AFAICT, HTTP/2 needs
  // this.)
  //
  // This method could be called in IO thread, return as quickly as possible.
  //
  // For optimization, the implementation may return a partially parsed message
  // here and left the rest to `TryParse(...)`, which is called in "worker"
  // thread.
  virtual MessageCutStatus TryCutMessage(std::string& buffer,
                                         std::unique_ptr<Message>* message) = 0;

  // If `TryCutMessage` has already done all parsing, this method could just
  // leave `message` untouched. The implementation is permitted (and likely need
  // to) replace `message` with a new object (probably of a different type).
  //
  // The message is dropped if the call fails. (The connection will be left
  // intact.)
  //
  // Called in "worker" thread.
  virtual bool TryParse(std::unique_ptr<Message>* message,
                        Controller* controller) = 0;

  // Serialize `message` to `buffer`.
  virtual void WriteMessage(const Message& message, std::string& buffer,
                            Controller* controller) = 0;
};

// TODO: what does this mean?
FLARE_DECLARE_CLASS_DEPENDENCY_REGISTRY(client_side_stream_protocol_registry,
                                        StreamProtocol);
FLARE_DECLARE_CLASS_DEPENDENCY_REGISTRY(server_side_stream_protocol_registry,
                                        StreamProtocol);

TINYRPC_DEFINE_ENUM_BITMASK_OPS(StreamProtocol::MessageCutStatus);

}  // namespace tinyRPC

// Define protocol by its class name.
#define FLARE_RPC_REGISTER_CLIENT_SIDE_STREAM_PROTOCOL(Name, Implementation)   \
  FLARE_REGISTER_CLASS_DEPENDENCY(flare::client_side_stream_protocol_registry, \
                                  Name, Implementation)

#define FLARE_RPC_REGISTER_SERVER_SIDE_STREAM_PROTOCOL(Name, Implementation)   \
  FLARE_REGISTER_CLASS_DEPENDENCY(flare::server_side_stream_protocol_registry, \
                                  Name, Implementation)

// Define protocol by its factory.
#define FLARE_RPC_REGISTER_CLIENT_SIDE_STREAM_PROTOCOL_FACTORY(Name, Factory) \
  FLARE_RPC_REGISTER_FACTORY_EX(flare::client_side_stream_protocol_registry,  \
                                Name, Factory)

#define FLARE_RPC_REGISTER_SERVER_SIDE_STREAM_PROTOCOL_FACTORY(Name, Factory) \
  FLARE_RPC_REGISTER_FACTORY_EX(flare::server_side_stream_protocol_registry,  \
                                Name, Factory)

// Define protocol by its name, with additional arguments bound to its
// constructor.
#define FLARE_RPC_REGISTER_CLIENT_SIDE_STREAM_PROTOCOL_ARG( \
    Name, Implementation, ...)                              \
  FLARE_REGISTER_CLASS_DEPENDENCY_FACTORY(                  \
      tinyRPC::client_side_stream_protocol_registry, Name,    \
      [] { return std::make_unique<Implementation>(__VA_ARGS__); })

#define FLARE_RPC_REGISTER_SERVER_SIDE_STREAM_PROTOCOL_ARG( \
    Name, Implementation, ...)                              \
  FLARE_REGISTER_CLASS_DEPENDENCY_FACTORY(                  \
      tinyRPC::server_side_stream_protocol_registry, Name,    \
      [] { return std::make_unique<Implementation>(__VA_ARGS__); })

#endif  
