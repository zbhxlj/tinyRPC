#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <thread>
#include <random>

#include "../../../include/gtest/gtest.h"
#include "SchedulingGroup.h"
#include "FiberEntity.h"
#include "Waitable.h"

namespace tinyRPC::fiber::detail{

static constexpr auto kYieldTime = 1000; 
static constexpr auto kFiberNum = 16;
static constexpr uint32_t kWorkerNum = 16;
void WorkerProcTest(SchedulingGroup* sg, std::size_t workerIndex){
  sg->EnterGroup(workerIndex);

  while (true) {
    FiberEntity* ready = sg->AcquireFiber();

    while (ready == nullptr) {
      ready = sg->WaitForFiber();
    }
    if (ready == SchedulingGroup::kSchedulingGroupShuttingDown) {
      break;
    }
    ready->Resume();
    ASSERT_EQ(GetCurrentFiberEntity(), GetMasterFiberEntity());
  }
  sg->LeaveGroup();
}

struct Context {
  std::atomic<std::size_t> executed_fibers{0};
  std::atomic<std::size_t> yields{0};
} context;

void FiberProc(Context* ctx) {
  auto sg = SchedulingGroup::Current();
  auto self = GetCurrentFiberEntity();
  std::atomic<std::size_t> yield_count_local{0};

  // It can't be.
  ASSERT_NE(self, GetMasterFiberEntity());

  for (int i = 0; i != kYieldTime; ++i) {
    ASSERT_EQ(FiberState::RUNNING, self->state_);
    sg->Yield(self);
    ++ctx->yields;
    ASSERT_EQ(self, GetCurrentFiberEntity());
    ++yield_count_local;
  }

  // The fiber is resumed by two worker concurrently otherwise.
  ASSERT_EQ(kYieldTime, yield_count_local);

  ++ctx->executed_fibers;
}

TEST(BasicRoutineTest, RunFibers) {
  context.executed_fibers = 0;
  context.yields = 0;

  LOG(INFO) << "Starting "<< kFiberNum <<" fibers.";

  auto scheduling_group =
      std::make_unique<SchedulingGroup>(kWorkerNum);
  std::thread workers[kWorkerNum];
  TimerWorker dummy(scheduling_group.get());
  scheduling_group->SetTimerWorker(&dummy);

  for (int i = 0; i != kFiberNum; ++i) {
    scheduling_group->StartFiber(
        CreateFiberEntity(scheduling_group.get(), [&] { FiberProc(&context); }, std::make_shared<ExitBarrier>())
    );
  }

  for (int i = 0; i != kWorkerNum; ++i) {
    workers[i] = std::thread(WorkerProcTest, scheduling_group.get(), i);
  }
  
  while (context.executed_fibers != kFiberNum) {
    std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(100));
  }
  scheduling_group->Stop();
  for (auto&& t : workers) {
    t.join();
  }
  ASSERT_EQ(kFiberNum, context.executed_fibers);
  ASSERT_EQ(kFiberNum * kYieldTime, context.yields);
}

TEST(BasicRoutineTest, WaitForRunFibers) {
  context.executed_fibers = 0;
  context.yields = 0;

  LOG(INFO) << "Starting "<< kFiberNum <<" fibers.";

  auto scheduling_group =
      std::make_unique<SchedulingGroup>(kWorkerNum);
  std::thread workers[kWorkerNum];
  TimerWorker dummy(scheduling_group.get());
  scheduling_group->SetTimerWorker(&dummy);

  for (int i = 0; i != kWorkerNum; ++i) {
    workers[i] = std::thread(WorkerProcTest, scheduling_group.get(), i);
  }

  std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(100));

  for (int i = 0; i != kFiberNum; ++i) {
    scheduling_group->StartFiber(
        CreateFiberEntity(scheduling_group.get(), [&] { FiberProc(&context); }, std::make_shared<ExitBarrier>())
    );
  }

  
  while (context.executed_fibers != kFiberNum) {
    std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(10));
  }
  scheduling_group->Stop();
  for (auto&& t : workers) {
    t.join();
  }
  ASSERT_EQ(kFiberNum, context.executed_fibers);
  ASSERT_EQ(kFiberNum * kYieldTime, context.yields);
}

std::atomic<std::size_t> switched{};

void SwitchToNewFiber(SchedulingGroup* sg, std::size_t left) {
  if (--left) {
    auto next = CreateFiberEntity(sg, [sg, left] {
      SwitchToNewFiber(sg, left);
    }, std::make_shared<ExitBarrier>());
    sg->SwitchTo(GetCurrentFiberEntity(), next);
  }
  ++switched;
}

TEST(BasicRoutineTest, SwitchToFiber) {
  switched = 0;
  google::FlagSaver fs;

  auto scheduling_group =
      std::make_unique<SchedulingGroup>(kWorkerNum);
  std::thread workers[kWorkerNum];
  TimerWorker dummy(scheduling_group.get());
  scheduling_group->SetTimerWorker(&dummy);

  for (int i = 0; i != kWorkerNum; ++i) {
    workers[i] = std::thread(WorkerProcTest, scheduling_group.get(), i);
  }

  scheduling_group->StartFiber(
        CreateFiberEntity(scheduling_group.get(), 
          [&] {  SwitchToNewFiber(scheduling_group.get(), kFiberNum);  }, 
          std::make_shared<ExitBarrier>())
    );

  while (switched != kFiberNum) {
    std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(100));
  }
  scheduling_group->Stop();
  for (auto&& t : workers) {
    t.join();
  }
  ASSERT_EQ(kFiberNum, switched);
}

TEST(BasicRoutineTest, WaitForFiberExit) {
  auto scheduling_group =
      std::make_unique<SchedulingGroup>(kWorkerNum);
  std::thread workers[kWorkerNum];
  TimerWorker timer_worker(scheduling_group.get());
  scheduling_group->SetTimerWorker(&timer_worker);

  for (int i = 0; i != kWorkerNum; ++i) {
    workers[i] = std::thread(WorkerProcTest, scheduling_group.get(), i);
  }
  timer_worker.Start();

  for (int k = 0; k != 10; ++k) {
    std::srand(time(nullptr));
    constexpr auto N = 1024;
    std::atomic<std::size_t> waited{};
    for (int i = 0; i != N; ++i) {
      auto f1 = CreateFiberEntity(
          scheduling_group.get(), [] {
            WaitableTimer wt(std::chrono::steady_clock::now() + static_cast<std::chrono::milliseconds>(10 * std::rand() % 10));
            wt.wait();
          }, std::make_shared<ExitBarrier>());
      auto f2 = CreateFiberEntity(scheduling_group.get(),
                                  [&, wc = f1-> exitBarrier_ ] {
                                    wc->Wait();
                                    ++waited;
                                  }, nullptr);
      if (std::rand() % 2 == 0) {
        scheduling_group->ReadyFiber(f1, {});
        scheduling_group->ReadyFiber(f2, {});
      } else {
        scheduling_group->ReadyFiber(f2, {});
        scheduling_group->ReadyFiber(f1, {});
      }
    }
    while (waited != N) {
      std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(10));
    }
  }
  scheduling_group->Stop();
  timer_worker.Stop();
  timer_worker.Join();
  for (auto&& t : workers) {
    t.join();
  }
}
template <class SG, class... Args>
[[nodiscard]] TimerPtr SetTimerAt(SG&& sg, Args&&... args) {
  auto tid = sg->CreateTimer(std::forward<Args>(args)...);
  sg->EnableTimer(tid);
  return tid;
}

void SleepyFiberProc(std::atomic<std::size_t>* leaving) {
  std::srand(time(nullptr));

  auto self = GetCurrentFiberEntity();
  auto sg = self->sg_;
  std::unique_lock lk(self->schedulerLock_);

  auto timer_cb = [self](auto) {
    std::unique_lock lk(self->schedulerLock_);
    self->state_ = FiberState::READY;
    self->sg_->ReadyFiber(self, std::move(lk));
  };
  auto timer_id =
      SetTimerAt(sg, std::chrono::steady_clock::now() + static_cast<std::chrono::milliseconds>(std::rand() % 10), timer_cb);

  sg->Halt(self, std::move(lk));
  sg->RemoveTimer(timer_id);
  ++*leaving;
}

TEST(BasicRoutineTest, SetTimer) {
  auto scheduling_group =
      std::make_unique<SchedulingGroup>(kWorkerNum);
  std::thread workers[kWorkerNum];
  
  std::atomic<std::size_t> leaving{0};

  TimerWorker timer_worker(scheduling_group.get());
  scheduling_group->SetTimerWorker(&timer_worker);

  for (int i = 0; i != 16; ++i) {
    workers[i] = std::thread(WorkerProcTest, scheduling_group.get(), i);
  }
  timer_worker.Start();

  constexpr auto N = 300;
  for (int i = 0; i != N; ++i) {
      scheduling_group->StartFiber(
        CreateFiberEntity(scheduling_group.get(), 
           [&] { SleepyFiberProc(&leaving);  }, 
           nullptr)
    );
  }
  while (leaving != N) {
    std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(10));
  }
  scheduling_group->Stop();
  timer_worker.Stop();
  timer_worker.Join();
  for (auto&& t : workers) {
    t.join();
  }
  ASSERT_EQ(N, leaving);
}

TEST(BasicRoutineTest, SetTimerPeriodic) {
  auto scheduling_group =
      std::make_unique<SchedulingGroup>(1);
  TimerWorker timer_worker(scheduling_group.get());
  scheduling_group->SetTimerWorker(&timer_worker);
  auto worker = std::thread(WorkerProcTest, scheduling_group.get(), 0);
  timer_worker.Start();

  std::atomic<std::size_t> called{};
  TimerPtr timer_id;

  scheduling_group->StartFiber(
        CreateFiberEntity(scheduling_group.get(), 
              [&] {
              auto cb = [&](auto) {
                if (called < 10) {
                  ++called;
                }
              };
              timer_id =
                  SetTimerAt(scheduling_group, std::chrono::steady_clock::now() , static_cast<std::chrono::nanoseconds>(10'000'000), cb);
            }, 
           nullptr)
    );

  while (called != 10) {
    std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(10));
  }
  scheduling_group->RemoveTimer(timer_id);
  scheduling_group->Stop();
  timer_worker.Stop();
  timer_worker.Join();
  worker.join();
  ASSERT_EQ(10, called);
}

} // namespace tinyRPC::fiber::detail