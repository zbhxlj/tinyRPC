#include "../../include/gtest/gtest.h"

#include "Fiber.h"
#include "Runtime.h"
#include "ThisFiber.h"

namespace tinyRPC::fiber{

using namespace std::literals;

namespace {

template <class F>
void RunAsFiber(F&& f) {
  fiber::StartRuntime();
  std::atomic<bool> done{};
  Fiber([&, f = std::forward<F>(f)] {
    f();
    done = true;
  }).detach();
  while (!done) {
    std::this_thread::sleep_for(1ms);
  }
  fiber::TerminateRuntime();
}

TEST(Fiber, StartFiberFromPthread) {
  RunAsFiber([&] {
    std::atomic<bool> called{};
    std::thread([&] {
      StartFiberFromPthread([&] {
        this_fiber::Yield();  // Would crash in pthread.
        ASSERT_TRUE(1);
        called = true;
      });
    }).join();
    while (!called.load()) {
      // NOTHING.
    }
  });
}

}  // namespace

} // namespace tinyRPC::fiber