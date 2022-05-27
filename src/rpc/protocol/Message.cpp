#include "Message.h"

namespace tinyRPC {

namespace {

class NullFactory : public MessageFactory {
 public:
  std::unique_ptr<Message> Create(Type, std::uint64_t, bool) const override {
    return nullptr;
  }
};

}  // namespace

const MessageFactory* MessageFactory::null_factory = new NullFactory();

}  // namespace tinyRPC
