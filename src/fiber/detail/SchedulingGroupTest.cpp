#include <chrono>
#include <cstddef>
#include <thread>
#include "../../../include/gtest/gtest.h"
#include "SchedulingGroup.h"
#include "FiberEntity.h"
#include "Waitable.h"

namespace tinyRPC::fiber::detail{

static constexpr auto kYieldTime = 1000; 
static const auto N = 16;

constexpr uint32_t workerNum = 16;
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

  LOG(INFO) << "Starting "<< N <<" fibers.";

  auto scheduling_group =
      std::make_unique<SchedulingGroup>(workerNum);
  std::thread workers[workerNum];
  TimerWorker dummy(scheduling_group.get());
  scheduling_group->SetTimerWorker(&dummy);

  for (int i = 0; i != N; ++i) {
    scheduling_group->StartFiber(
        CreateFiberEntity(scheduling_group.get(), [&] { FiberProc(&context); }, std::make_shared<ExitBarrier>())
    );
  }

  for (int i = 0; i != workerNum; ++i) {
    workers[i] = std::thread(WorkerProcTest, scheduling_group.get(), i);
  }
  
  while (context.executed_fibers != N) {
    std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(100));
  }
  scheduling_group->Stop();
  for (auto&& t : workers) {
    t.join();
  }
  ASSERT_EQ(N, context.executed_fibers);
  ASSERT_EQ(N * kYieldTime, context.yields);
}

TEST(BasicRoutineTest, WaitForRunFibers) {
  context.executed_fibers = 0;
  context.yields = 0;

  LOG(INFO) << "Starting "<< N <<" fibers.";

  auto scheduling_group =
      std::make_unique<SchedulingGroup>(workerNum);
  std::thread workers[workerNum];
  TimerWorker dummy(scheduling_group.get());
  scheduling_group->SetTimerWorker(&dummy);

  for (int i = 0; i != workerNum; ++i) {
    workers[i] = std::thread(WorkerProcTest, scheduling_group.get(), i);
  }

  std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(100));

  for (int i = 0; i != N; ++i) {
    scheduling_group->StartFiber(
        CreateFiberEntity(scheduling_group.get(), [&] { FiberProc(&context); }, std::make_shared<ExitBarrier>())
    );
  }

  
  while (context.executed_fibers != N) {
    std::this_thread::sleep_for(static_cast<std::chrono::milliseconds>(10));
  }
  scheduling_group->Stop();
  for (auto&& t : workers) {
    t.join();
  }
  ASSERT_EQ(N, context.executed_fibers);
  ASSERT_EQ(N * kYieldTime, context.yields);
}

} // namespace tinyRPC::fiber::detail