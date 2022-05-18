#include <chrono>
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

// Stealing seems not very effective.
TEST(Fiber, SchedulingGroupLocal) {
  RunAsFiber([] {
    constexpr auto N = 10000;

    std::atomic<std::size_t> run{};
    std::vector<Fiber> fs(N);
    std::atomic<bool> stop{};

    for (std::size_t i = 0; i != N; ++i) {
      auto sgi = i % fiber::GetSchedulingGroupCount();
      auto cb = [&, sgi] {
        while (!stop) {
          ASSERT_EQ(sgi, fiber::NearestSchedulingGroupIndex());
          this_fiber::Yield();
        }
        ++run;
      };
      fs[i] = Fiber(Fiber::Attributes{.sg = sgi,.local = true},cb);
    }

    auto start = std::chrono::steady_clock::now();

    while (start + 3s > std::chrono::steady_clock::now()) {
      std::this_thread::sleep_for(1ms);

      // Wake up workers in each scheduling group (for them to be thieves.).
      auto count = fiber::GetSchedulingGroupCount();
      for (std::size_t i = 0; i != count; ++i) {
        Fiber(Fiber::Attributes{.sg = i}, [] {}).join();
      }
    }

    stop = true;
    for (auto&& e : fs) {
      ASSERT_TRUE(e.joinable());
      e.join();
    }

    ASSERT_EQ(N, run);
  });
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