#ifndef _SRC_RPC_PROTOCOL_CONTROLLER_H_
#define _SRC_RPC_PROTOCOL_CONTROLLER_H_

#include <memory>
#include <string>
#include <utility>

namespace tinyRPC {

// The `Controller` controls a single RPC. It also serves as "context" for
// protocols to pass / retrieve information between its methods.
class Controller {
 public:
  virtual ~Controller() = default;
};

// Factory for creating controllers.
class ControllerFactory {
 public:
  virtual ~ControllerFactory() = default;

  // Create a new controller.
  //
  // TODO(luobogao): Consider use object pool here.
  virtual std::unique_ptr<Controller> Create() const = 0;

  // A null factory that always returns `nullptr`.
  //
  // Note that unless the protocol you're implementing does not uses
  // "controller", you shouldn't use this null factory.
  static const ControllerFactory* null_factory;
};

}  // namespace tinyRPC

#endif  
