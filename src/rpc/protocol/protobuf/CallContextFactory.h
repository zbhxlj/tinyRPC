#ifndef _SRC_RPC_PROTOCOL_PROTOBUF_CALL_CONTEXT_FACTORY_H_
#define _SRC_RPC_PROTOCOL_PROTOBUF_CALL_CONTEXT_FACTORY_H_

#include "../Controller.h"

namespace tinyRPC::protobuf {

class PassiveCallContextFactory : public ControllerFactory {
 public:
  std::unique_ptr<Controller> Create() const override;
};

extern PassiveCallContextFactory passive_call_context_factory;

}  // namespace tinyPRC::protobuf

#endif  
