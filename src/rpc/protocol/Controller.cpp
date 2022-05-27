#include "Controller.h"

namespace tinyRPC {

namespace {

class NullFactory : public ControllerFactory {
 public:
  std::unique_ptr<Controller> Create() const override {
    return std::make_unique<Controller>();
  }
};

}  // namespace

const ControllerFactory* ControllerFactory::null_factory = new NullFactory;

}  // namespace tinyRPC
