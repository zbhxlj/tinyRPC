#include "glog/logging.h"

#include "FiberEntity.h"

namespace tinyRPC::fiber::detail{

void FiberEntity::Resume() noexcept {
  auto caller = GetCurrentFiberEntity();
  CHECK_NE(caller, this) << "Calling `Resume()` on self is undefined." << std::endl;

  jump_context(&caller->stackSaveBuffer_, stackSaveBuffer_, this);

  SetCurrentFiberEntity(caller);  // The caller has back.

  // Check for pending `OnResume`.
  DestructiveRunCallbackOpt(&caller->resumeProc_);
}

}