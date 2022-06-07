#include "CallContextFactory.h"
#include "CallContext.h"

namespace tinyRPC::protobuf {

PassiveCallContextFactory passive_call_context_factory;

std::unique_ptr<Controller> PassiveCallContextFactory::Create() const {
  return std::make_unique<PassiveCallContext>();
}

}  // namespace tinyRPC::protobuf
