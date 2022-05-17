#include <chrono>
#include "TimerWorker.h"
#include "SchedulingGroup.h"
#include "../../../include/gtest/gtest.h"

namespace tinyRPC::fiber::detail{

namespace {

template <class SG, class... Args>
[[nodiscard]] TimerPtr SetTimerAt(SG&& sg, Args&&... args) {
  auto tid = sg->CreateTimer(std::forward<Args>(args)...);
  sg->EnableTimer(tid);
  return tid;
}

}  // namespace

TEST(TimerWorker, EarlyTimer) {
  std::atomic<bool> called  {false};

  auto scheduling_group =
      std::make_unique<SchedulingGroup>(1);
  TimerWorker worker(scheduling_group.get());
  scheduling_group->SetTimerWorker(&worker);
  std::thread t = std::thread([&scheduling_group, &called] {
    scheduling_group->EnterGroup(0);

    (void)SetTimerAt(scheduling_group,
                     std::chrono::steady_clock::time_point::min(),
                     [&](auto tid) {
                       scheduling_group->RemoveTimer(tid);
                       called = true;
                     });
    std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(10));
    scheduling_group->LeaveGroup();
  });

  worker.Start();
  t.join();
  worker.Stop();
  worker.Join();

  ASSERT_TRUE(called);
}

TEST(TimerWorker, SetTimerInTimerContext) {
  std::atomic<bool> called {false};

  auto scheduling_group =
      std::make_unique<SchedulingGroup>(1);
  TimerWorker worker(scheduling_group.get());
  scheduling_group->SetTimerWorker(&worker);
  std::thread t = std::thread([&scheduling_group, &called] {
    scheduling_group->EnterGroup(0);

    auto timer_cb = [&](auto tid) {
      auto timer_cb2 = [&, tid](auto tid2) mutable {
        scheduling_group->RemoveTimer(tid);
        scheduling_group->RemoveTimer(tid2);
        called = true;
      };
      (void)SetTimerAt(scheduling_group,
                       std::chrono::steady_clock::time_point{}, timer_cb2);
    };
    (void)SetTimerAt(scheduling_group,
                     std::chrono::steady_clock::time_point::min(), timer_cb);
    std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(100));
    scheduling_group->LeaveGroup();
  });

  worker.Start();
  t.join();
  worker.Stop();
  worker.Join();

  ASSERT_TRUE(called);
}

std::atomic<std::size_t> timer_set, timer_removed;

TEST(TimerWorker, Torture) {
  constexpr auto N = 100'000;
  // If there are enough threads(may be > 4), we will core dump this test
  // due to invalid access when timerWorker try to access thread_local
  // timer_queue but the worker thread already eixts. 
  // This concerns about gracefully shut down, but I think it does not matter
  // much.
  // FIXME: Gracefully shutdown.
  constexpr auto T = 5;

  auto scheduling_group =
      std::make_unique<SchedulingGroup>(T);
  TimerWorker worker(scheduling_group.get());
  scheduling_group->SetTimerWorker(&worker);
  std::thread ts[T];

  for (int i = 0; i != T; ++i) {
    std::srand(time(NULL));
    ts[i] = std::thread([i, &scheduling_group] {
      scheduling_group->EnterGroup(i);

      for (int j = 0; j != N; ++j) {
        auto timeout = std::chrono::steady_clock::now() + static_cast<std::chrono::microseconds>(std::rand() % 2'000'000);
        if (j % 2 == 0) {
          // In this case we set a timer and let it fire.
          (void)SetTimerAt(scheduling_group, timeout,
                           [&scheduling_group](auto timer_id) {
                             scheduling_group->RemoveTimer(timer_id);
                             ++timer_removed;
                           });  // Indirectly calls `TimerWorker::AddTimer`.
          ++timer_set;
        } else {
          // In this case we set a timer and cancel it sometime later.
          auto timer_id = SetTimerAt(scheduling_group, timeout, [](auto) {});
          (void)SetTimerAt(scheduling_group,
                           std::chrono::steady_clock::now() + static_cast<std::chrono::milliseconds>(std::rand() % 1000),
                           [timer_id, &scheduling_group](auto self) mutable {
                             scheduling_group->RemoveTimer(timer_id);
                             scheduling_group->RemoveTimer(self);
                             ++timer_removed;
                           });
          ++timer_set;
        }
        if (j % 10000 == 0) {
          std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(100));
        }
      }

      // Wait until all timers have been consumed. Otherwise if we leave the
      // thread too early, `TimerWorker` might incurs use-after-free when
      // accessing our thread-local timer queue.
      while (timer_removed != timer_set) {
        std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(100));
      }
      scheduling_group->LeaveGroup();
    });
  }

  worker.Start();

  for (auto&& t : ts) {
    t.join();
  }
  worker.Stop();
  worker.Join();

  ASSERT_EQ(timer_set, timer_removed);
  ASSERT_EQ(N * T, timer_set);
}

} // namespace tinyRPC::fiber::detail