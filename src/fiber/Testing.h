#include "detail/FiberEntity.h"
#include "detail/SchedulingGroup.h"
#include "Fiber.h"
#include "Runtime.h"

namespace tinyRPC::fiber::testing{

template <class F>
void RunAsFiber(F&& f) {
  fiber::StartRuntime();
  std::atomic<bool> done{};
  Fiber([&, f = std::forward<F>(f)] {
    f();
    done = true;
  }).detach();
  while (!done) {
    std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
  fiber::TerminateRuntime();
}

} // namespace tinyRPC::fiber::testing