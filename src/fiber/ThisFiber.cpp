#include <chrono>
#include "../../include/glog/logging.h"

#include "ThisFiber.h"
#include "detail/FiberEntity.h"
#include "detail/SchedulingGroup.h"
#include "detail/Waitable.h"
namespace tinyRPC::this_fiber{

void Yield() {
  auto self = fiber::detail::GetCurrentFiberEntity();
  CHECK(self) << "this_fiber::Yield may only be called in fiber environment.";
  self->sg_->Yield(self);
}

void SleepUntil(std::chrono::steady_clock::time_point expires_at) {
  fiber::detail::WaitableTimer wt(expires_at);
  wt.wait();
}

void SleepFor(std::chrono::nanoseconds expires_in) {
  SleepUntil(std::chrono::steady_clock::now() + expires_in);
}

} // namespace tinyRPC::this_fiber