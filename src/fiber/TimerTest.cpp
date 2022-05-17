#include <atomic>
#include <thread>

#include "../../include/gtest/gtest.h"

#include "Timer.h"
#include "Testing.h"
#include "ThisFiber.h"

using namespace std::literals;

namespace tinyRPC::fiber {

TEST(Timer, SetTimer) {
  testing::RunAsFiber([] {
    auto start = std::chrono::steady_clock::now();
    std::atomic<bool> done{};
    auto timer_id = SetTimer(start + 100ms, [&]() {
      ASSERT_NEAR((std::chrono::steady_clock::now() - start) / 1ms, 100ms / 1ms, 10);
      done = true;
    });
    while (!done) {
      std::this_thread::sleep_for(1ms);
    }
    KillTimer(timer_id);
  });
}

TEST(Timer, SetPeriodicTimer) {
  testing::RunAsFiber([] {
    auto start = std::chrono::steady_clock::now();
    std::atomic<std::size_t> called{};
    auto timer_id = SetTimer(start + 100ms, 10ms, [&]() {
      ASSERT_NEAR((std::chrono::steady_clock::now() - start) / 1ms,
                  (100ms + called.load() * 10ms) / 1ms, 10);
      ++called;
    });
    while (called != 10) {
      std::this_thread::sleep_for(1ms);
    }
    KillTimer(timer_id);

    // It's possible that the timer callback is running when `KillTimer` is
    // called, so wait for it to complete.
    std::this_thread::sleep_for(500ms);
  });
}

}  // namespace tinyRPC::fiber
