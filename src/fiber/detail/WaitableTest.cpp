#include <chrono>
#include <thread>
#include <type_traits>
#include <numeric>

#include "../../../include/gtest/gtest.h"

#include "Waitable.h"
#include "SchedulingGroup.h"
#include "FiberWorker.h"

using namespace std::chrono_literals;

namespace tinyRPC::fiber::detail{


namespace {

void Sleep(std::chrono::nanoseconds ns) {
  WaitableTimer wt(std::chrono::steady_clock::now() + ns);
  wt.wait();
}

std::uint64_t Random(std::uint64_t num){
  std::srand(time(NULL));
  return rand() % num;
}

template <class F>
void StartFiberEntityInGroup(SchedulingGroup* sg, 
                          F&& startFunc, std::shared_ptr<ExitBarrier> barrier = nullptr){
  sg->StartFiber(CreateFiberEntity(sg, std::move(startFunc), barrier));
}

template <class F>
void RunInFiber(std::size_t times, F cb) {
  std::atomic<std::size_t> called{};
  auto sg = std::make_unique<SchedulingGroup>(16);
  TimerWorker timer_worker(sg.get());
  sg->SetTimerWorker(&timer_worker);
  std::deque<FiberWorker> workers;

  for (int i = 0; i != 16; ++i) {
    workers.emplace_back(sg.get(), i).Start();
  }
  timer_worker.Start();
  for (int i = 0; i != times; ++i) {
    StartFiberEntityInGroup(sg.get(), [&, i] {
      cb(i);
      ++called;
    });
  }
  while (called != times) {
    std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(100));
  }
  sg->Stop();
  timer_worker.Stop();
  std::this_thread::sleep_for(100ms);
  for (auto&& w : workers) {
    w.Join();
  }
  timer_worker.Join();
}

}

TEST(DISABLED_WaitableTest, WaitableTimer) {
  RunInFiber(100, [](auto) {
    auto now = std::chrono::steady_clock::now();
    WaitableTimer wt(now + 1s);
    wt.wait();
    EXPECT_NEAR(1s / 1ms, 
      (std::chrono::steady_clock::now() - now) / 1ms, 100);
  });
}

TEST(WaitableTest, Mutex) {
  for (int i = 0; i != 10; ++i) {
    Mutex m;
    int value = 0;
    RunInFiber(10000, [&](auto) {
      std::scoped_lock _(m);
      ++value;
    });
    ASSERT_EQ(10000, value);
  }
}

TEST(WaitableTest, ConditionVariable) {
  constexpr auto N = 100;

  for (int i = 0; i != 10; ++i) {
    Mutex m[N];
    ConditionVariable cv[N];
    std::queue<int> queue[N];
    std::atomic<std::size_t> read{}, write{};
    int sum[N] = {};

    // We, in fact, are passing data between two scheduling groups.
    //
    // This should work anyway.
    auto prods = std::thread([&] {
      RunInFiber(N, [&](auto index) {
        auto to = Random(N - 1);
        std::scoped_lock _(m[to]);
        queue[to].push(index);
        cv[to].notify_one();
        ++write;
      });
    });
    auto signalers = std::thread([&] {
      RunInFiber(N, [&](auto index) {
        std::unique_lock lk(m[index]);
        bool exit = false;
        while (!exit) {
          auto&& mq = queue[index];
          cv[index].wait(lk, [&] { return !mq.empty(); });
          ASSERT_TRUE(lk.owns_lock());
          while (!mq.empty()) {
            if (mq.front() == -1) {
              exit = true;
              break;
            }
            sum[index] += mq.front();
            ++read;
            mq.pop();
          }
        }
      });
    });
    prods.join();
    RunInFiber(N, [&](auto index) {
      std::scoped_lock _(m[index]);
      queue[index].push(-1);
      cv[index].notify_one();
    });
    signalers.join();
    ASSERT_EQ((N - 1) * N / 2,
              std::accumulate(std::begin(sum), std::end(sum), 0));
  }
}

TEST(WaitableTest, ConditionVariable2) {
  constexpr auto N = 10;

  for (int i = 0; i != 50; ++i) {
    Mutex m[N];
    ConditionVariable cv[N];
    bool f[N] = {};
    std::atomic<int> sum{};
    auto prods = std::thread([&] {
      RunInFiber(N, [&](auto index) {
        Sleep(Random(10) * 1ms);
        std::scoped_lock _(m[index]);
        f[index] = true;
        cv[index].notify_one();
      });
    });
    auto signalers = std::thread([&] {
      RunInFiber(N, [&](auto index) {
        Sleep(Random(10) * 1ms);
        std::unique_lock lk(m[index]);
        cv[index].wait(lk, [&] { return f[index]; });
        ASSERT_TRUE(lk.owns_lock());
        sum += index;
      });
    });
    prods.join();
    signalers.join();
    ASSERT_EQ((N - 1) * N / 2, sum);
  }
}

TEST(WaitableTest, ConditionVariableNoTimeout) {
  constexpr auto N = 100;
  std::atomic<std::size_t> done{};
  Mutex m;
  ConditionVariable cv;
  auto waiter = std::thread([&] {
    RunInFiber(N, [&](auto index) {
      std::unique_lock lk(m);
      done += cv.wait_until(lk, std::chrono::steady_clock::now() + 100s);
    });
  });
  std::thread([&] {
    RunInFiber(1, [&](auto index) {
      while (done != N) {
        cv.notify_all();
      }
    });
  }).join();
  ASSERT_EQ(N, done);
  waiter.join();
}

TEST(WaitableTest, ConditionVariableTimeout) {
  constexpr auto N = 100;
  std::atomic<std::size_t> timed_out{};
  Mutex m;
  ConditionVariable cv;
  RunInFiber(N, [&](auto index) {
    std::unique_lock lk(m);
    timed_out += !cv.wait_until(lk, std::chrono::steady_clock::now() + 1ms);
  });
  ASSERT_EQ(N, timed_out);
}

TEST(WaitableTest, ConditionVariableRace) {
  constexpr auto N = 10;

  for (int i = 0; i != 5; ++i) {
    Mutex m;
    ConditionVariable cv;
    std::atomic<int> sum{};
    auto prods = std::thread([&] {
      RunInFiber(N, [&](auto index) {
        for (int j = 0; j != 100; ++j) {
          Sleep(Random(100) * 1us);
          std::scoped_lock _(m);
          cv.notify_all();
        }
      });
    });
    auto signalers = std::thread([&] {
      RunInFiber(N, [&](auto index) {
        for (int j = 0; j != 100; ++j) {
          std::unique_lock lk(m);
          cv.wait_until(lk, std::chrono::steady_clock::now() + 50us);
          ASSERT_TRUE(lk.owns_lock());
        }
        sum += index;
      });
    });
    prods.join();
    signalers.join();
    ASSERT_EQ((N - 1) * N / 2, sum);
  }
}

TEST(WaitableTest, ExitBarrier) {
  constexpr auto N = 100;

  for (int i = 0; i != 10; ++i) {
    std::deque<ExitBarrier> l;
    std::atomic<std::size_t> waited{};

    for (int j = 0; j != N; ++j) {
      l.emplace_back();
    }

    auto counters = std::thread([&] {
      RunInFiber(N, [&](auto index) {
        Sleep(Random(10) * 1ms);
        l[index].UnsafeCountDown(l[index].GrabLock());
      });
    });
    auto waiters = std::thread([&] {
      RunInFiber(N, [&](auto index) {
        Sleep(Random(10) * 1ms);
        l[index].Wait();
        ++waited;
      });
    });
    counters.join();
    waiters.join();
    ASSERT_EQ(N, waited);
  }
}


} // namespace tinyRPC::fiber::detail