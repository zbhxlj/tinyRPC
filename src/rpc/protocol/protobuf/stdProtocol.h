#ifndef _SRC_RPC_PROTOCOL_PROTOBUF_STD_PROTOCOL_H_
#define _SRC_RPC_PROTOCOL_PROTOBUF_STD_PROTOCOL_H_

#include "../StreamProtocol.h"

namespace tinyRPC::protobuf {

// struct Header {  // All numbers are in network byte order.
//   __le32 magic;  // 'F', 'R', 'P', 'C'
//   __le32 meta_size;  // Size of meta.
//   __le32 msg_size;  // Size of message.
//   __le32 att_size;  // Size of attachment.
// };
//
// Wire format:  [Header][meta][payload][attachment]

class StdProtocol : public StreamProtocol {
 public:
  explicit StdProtocol(bool server_side) : server_side_(server_side) {}

  const Characteristics& GetCharacteristics() const override;

  // Get ErrorMessageFactory.
  const MessageFactory* GetMessageFactory() const override;
  // Get ClientCallContext.
  const ControllerFactory* GetControllerFactory() const override;

  // Examine `buffer` and extract message.
  // Parse until meta, then leave remains to TryParse in another fiber.
  MessageCutStatus TryCutMessage(std::string& buffer,
                                 std::unique_ptr<Message>* message) override;

  // Continue to parse the whole message.
  bool TryParse(std::unique_ptr<Message>* message,
                Controller* controller) override;

  // Serialize `message` into `buffer`.
  void WriteMessage(const Message& message, std::string& buffer,
                    Controller* controller) override;

 private:
  bool server_side_;
};

}  // namespace tinyRPC::protobuf

#endif  
