#include <chrono>
#include "FiberEntity.h"
#include "FiberWorker.h"
#include "../../../include/gtest/gtest.h"
#include "SchedulingGroup.h"
namespace tinyRPC::fiber::detail{

TEST(FiberWorker, ExecuteFiber) {
  std::atomic<std::size_t> executed{0};
  auto sg = std::make_unique<SchedulingGroup>(16);
  TimerWorker dummy(sg.get());
  sg->SetTimerWorker(&dummy);
  std::deque<FiberWorker> workers;

  for (int i = 0; i != 16; ++i) {
    workers.emplace_back(sg.get(), i).Start();
  }

  sg->StartFiber(CreateFiberEntity(sg.get(),[&] {
    ++executed;
  }, nullptr));

  sg->Stop();
  for (auto&& w : workers) {
    w.Join();
  }

  ASSERT_EQ(1, executed);
}

TEST(FiberWorker, StealFiber) {
  std::atomic<std::size_t> executed{0};
  auto sg = std::make_unique<SchedulingGroup>(16);
  auto sg2 = std::make_unique<SchedulingGroup>(1);
  TimerWorker dummy(sg.get());
  sg->SetTimerWorker(&dummy);
  std::deque<FiberWorker> workers;

  sg2->StartFiber(CreateFiberEntity(sg2.get(),[&] {
    ++executed;
  }, nullptr));


  for (int i = 0; i != 16; ++i) {
    auto&& w = workers.emplace_back(sg.get(), i);
    w.AddForeignSchedulingGroup(sg2.get(), 1);
    w.Start();
  }

  while (!executed) {
    // To wake worker up.
    sg->StartFiber(CreateFiberEntity(sg.get(),[&] {}, nullptr));
    std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(1));
  }
  sg->Stop();
  for (auto&& w : workers) {
    w.Join();
  }

  ASSERT_EQ(1, executed);
}

// Run too long, disable it.
TEST(FiberWorker, Torture) {
  constexpr auto T = 64;
  // Setting it too large cause `vm.max_map_count` overrun.
  constexpr auto N = 32768;
  constexpr auto P = 128;
  for (int i = 0; i != 50; ++i) {
    std::atomic<std::size_t> executed{0};
    auto sg = std::make_unique<SchedulingGroup>(T);
    TimerWorker dummy(sg.get());
    sg->SetTimerWorker(&dummy);
    std::deque<FiberWorker> workers;

    for (int i = 0; i != T; ++i) {
      workers.emplace_back(sg.get(), i).Start();
    }

    // Concurrently create fibers.
    std::thread prods[P];
    for (auto&& t : prods) {
      t = std::thread([&] {
        constexpr auto kChildren = 32;
        static_assert(N % P == 0 && (N / P) % kChildren == 0);
        for (int i = 0; i != N / P / kChildren; ++i) {
            sg->StartFiber(CreateFiberEntity(
              sg.get(), [&] {
                ++executed;
                for (auto j = 0; j != kChildren - 1 /* minus itself */; ++j) {
                    sg->StartFiber(CreateFiberEntity(sg.get(),[&] {++executed;}, nullptr));
                }
              }, nullptr));
        }
      });
    }

    for (auto&& t : prods) {
      t.join();
    }
    while (executed != N) {
      std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(100));
    }
    sg->Stop();
    for (auto&& w : workers) {
      w.Join();
    }

    ASSERT_EQ(N, executed);
  }
}

} // namespace tinyRPC::fiber::detail