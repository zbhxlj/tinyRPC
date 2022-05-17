#include "FiberLocal.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "../../include/gtest/gtest.h"

#include "../base/base.h"
#include "Testing.h"
#include "Fiber.h"
#include "ThisFiber.h"

using namespace std::literals;

namespace tinyRPC::fiber {

// This test concerns about cpu affinity and migration.
// Omit it for now.
// TEST(ThisFiber, Yield) {
//   fiber::testing::RunAsFiber([] {
//     for (int k = 0; k != 1; ++k) {
//       constexpr auto N = 1;

//       std::atomic<std::size_t> run{};
//       std::atomic<bool> ever_switched_thread{};
//       std::vector<Fiber> fs(N);

//       for (int i = 0; i != N; ++i) {
//         fs[i] = Fiber([&run, &ever_switched_thread] {
//           // `Yield()`
//           auto tid = std::this_thread::get_id();
//           while (tid == std::this_thread::get_id()) {
//             this_fiber::Yield();
//           }
//           ever_switched_thread = true;
//           ++run;
//         });
//       }

//       for (auto&& e : fs) {
//         ASSERT_TRUE(e.joinable());
//         e.join();
//       }

//       ASSERT_EQ(N, run);
//       ASSERT_TRUE(ever_switched_thread);
//     }
//   });
// }

TEST(ThisFiber, Sleep) {
  fiber::testing::RunAsFiber([] {
    for (int k = 0; k != 10; ++k) {
      // Don't run too many fibers here, waking pthread worker up is costly and
      // incurs delay. With too many fibers, that delay fails the UT (we're
      // testing timer delay here).
      constexpr auto N = 100;

      std::atomic<std::size_t> run{};
      std::vector<Fiber> fs(N);

      for (int i = 0; i != N; ++i) {
        fs[i] = Fiber([&run] {
          // `SleepFor()`
          auto sleep_for = Random(100) * 1ms;
          auto start = std::chrono::system_clock::now();  // Used system_clock intentionally.
          this_fiber::SleepFor(sleep_for);
          ASSERT_NEAR((std::chrono::system_clock::now() - start) / 1ms, sleep_for / 1ms, 30);

          // `SleepUntil()`
          auto sleep_until = std::chrono::steady_clock::now() + Random(100) * 1ms;
          this_fiber::SleepUntil(sleep_until);
          ASSERT_NEAR((std::chrono::steady_clock::now().time_since_epoch()) / 1ms,
                      sleep_until.time_since_epoch() / 1ms, 30);

          ++run;
        });
      }

      for (auto&& e : fs) {
        ASSERT_TRUE(e.joinable());
        e.join();
      }

      ASSERT_EQ(N, run);
    }
  });
}

}  // namespace tinyRPC::this_fiber
