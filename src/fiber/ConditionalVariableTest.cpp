#include <chrono>

#include "../base/Random.h"
#include "../../include/gtest/gtest.h"
#include "ConditionalVariable.h"
#include "Testing.h"
#include "ThisFiber.h"

namespace tinyRPC::fiber{

using namespace std::chrono_literals;

TEST(ConditionalVariable, All){
    testing::RunAsFiber([] {
    for (int k = 0; k != 10; ++k) {
      constexpr auto N = 600;

      std::atomic<std::size_t> run{};
      Mutex lock[N];
      fiber::ConditionVariable cv[N];
      bool set[N];
      std::vector<Fiber> prod(N);
      std::vector<Fiber> cons(N);

      for (int i = 0; i != N; ++i) {
        prod[i] = Fiber([&run, i, &cv, &lock, &set]() {
          this_fiber::SleepFor(Random(20) * 1ms);
          std::unique_lock lk(lock[i]);
          cv[i].wait(lk, [&] { return set[i]; });
          ++run;
        });
        cons[i] = Fiber([&run, i, &cv, &lock, &set]() {
          this_fiber::SleepFor(Random(20) * 1ms);
          std::scoped_lock _(lock[i]);
          set[i] = true;
          cv[i].notify_one();
          ++run;
        });
      }

      for (auto&& e : prod) {
        ASSERT_TRUE(e.joinable());
        e.join();
      }
      for (auto&& e : cons) {
        ASSERT_TRUE(e.joinable());
        e.join();
      }

      ASSERT_EQ(N * 2, run);
    }
  });
}

} // namespace tinyRPC::fiber
