#include "../../include/gtest/gtest.h"
#include "../../include/gmock/gmock.h"
#include "../base/Random.h"

#include "FiberLocal.h"
#include "Testing.h"
#include "ThisFiber.h"

namespace tinyRPC::fiber{

using namespace std::chrono_literals;
TEST(DISABLED_FiberLocal, All) {
  for (int k = 0; k != 10; ++k) {
    fiber::testing::RunAsFiber([] {
      static FiberLocal<int> fls;
      static FiberLocal<int> fls2;
      static FiberLocal<double> fls3;
      static FiberLocal<std::vector<int>> fls4;
      constexpr auto N = 10;

      std::atomic<std::size_t> run{};
      std::vector<Fiber> fs(N);

      for (int i = 0; i != N; ++i) {
        fs[i] = Fiber([i, &run] {
          *fls = i;
          *fls2 = i * 2;
          *fls3 = i + 3;
          fls4->push_back(123);
          fls4->push_back(456);
          while (Random(20) > 1) {
            this_fiber::SleepFor(Random(1000) * 1us);
            ASSERT_EQ(i, *fls);
            ASSERT_EQ(i * 2, *fls2);
            ASSERT_EQ(i + 3, *fls3);
            ASSERT_THAT(*fls4, ::testing::ElementsAre(123, 456));
          } 
          ++run;
        });
      }

      for (auto&& e : fs) {
        ASSERT_TRUE(e.joinable());
        e.join();
      }

      ASSERT_EQ(N, run);
    });
  }
}

} // namespace tinyRPC::fiber