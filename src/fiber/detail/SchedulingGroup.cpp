#include <unistd.h>
#include <linux/futex.h>
#include <syscall.h>
#include <cstdint>
#include <mutex>

#include "SchedulingGroup.h"
#include "FiberEntity.h"
#include "../../base/ScopedDeferred.h"
#include "../../../include/glog/logging.h"

namespace tinyRPC::fiber::detail{

thread_local SchedulingGroup* SchedulingGroup::current_{nullptr};

class SchedulingGroup::WaitSlot {
 public:
  void Wake() noexcept {
    if (wakeup_count_.fetch_add(1) == 0) {
      CHECK(syscall(SYS_futex, &wakeup_count_, FUTEX_WAKE_PRIVATE, 1, 0, 0, 0) >= 0);
    }
    CHECK_GE(wakeup_count_, 0);
  }

  void Wait() noexcept {
    if (wakeup_count_.fetch_sub(1) == 1) {
      do {
        auto rc =
            syscall(SYS_futex, &wakeup_count_, FUTEX_WAIT_PRIVATE, 0, 0, 0, 0);
        CHECK(rc == 0 || errno == EAGAIN);
      } while (wakeup_count_ == 0);
    }
    CHECK_GT(wakeup_count_, 0);
  }

  void PersistentWake() noexcept {
    wakeup_count_.store(0x4000'0000);
    CHECK(syscall(SYS_futex, &wakeup_count_, FUTEX_WAKE_PRIVATE, INT_MAX, 0, 0, 0) >= 0);
  }

 private:
  // `futex` requires this.
  static_assert(sizeof(std::atomic<int>) == sizeof(int));

  std::atomic<int> wakeup_count_{1};
};

thread_local std::size_t SchedulingGroup::workerIndex_ = kUninitializedWorkerIndex;

SchedulingGroup::SchedulingGroup(std::size_t size)
    : groupSize_(size) {
    waitSlots_ = std::make_unique<WaitSlot[]>(groupSize_);
}

SchedulingGroup::~SchedulingGroup() = default;

FiberEntity* SchedulingGroup::AcquireFiber() noexcept {
  std::scoped_lock lk(lock_);
  if (!readyFiberQueue_.empty()) {
    auto rc = readyFiberQueue_.front();
    readyFiberQueue_.pop();
    std::scoped_lock _(rc->schedulerLock_);

    CHECK(rc->state_ == FiberState::READY);
    rc->state_ = FiberState::RUNNING;

    return rc;
  }
  return stopped_ ? kSchedulingGroupShuttingDown : nullptr;
}

FiberEntity* SchedulingGroup::WaitForFiber() noexcept {
  CHECK_NE(workerIndex_, kUninitializedWorkerIndex);
  CHECK_LT(workerIndex_, groupSize_);
  auto mask = 1ULL << workerIndex_;

  while (true) {
    ScopedDeferred _([&] {
        sleepingWorkers_.fetch_and(~mask);
    });

    CHECK_EQ(sleepingWorkers_.fetch_or(mask) & mask, 0);
    
    if (auto f = AcquireFiber()) {
        if ((sleepingWorkers_.fetch_and(~mask) &mask) == 0) {
            WakeUpOneWorker();
        }
        return f;
      }

    waitSlots_[workerIndex_].Wait();

    if (auto f = AcquireFiber()) {
      return f;
    } 
  }
}

FiberEntity* SchedulingGroup::RemoteAcquireFiber() noexcept {
  std::scoped_lock lk(lock_);
  if (auto rc = readyFiberQueue_.front()) {
    readyFiberQueue_.pop();
    std::scoped_lock _(rc->schedulerLock_);

    CHECK(rc->state_ == FiberState::READY);
    rc->state_ = FiberState::RUNNING;

    rc->sg_ = Current();
    return rc;
  }
  return nullptr;
}

void SchedulingGroup::StartFiber(FiberEntity* fiberEntity) noexcept {
  QueueRunnableEntity(fiberEntity);
}

void SchedulingGroup::ReadyFiber(FiberEntity* fiberEntity, std::unique_lock<SpinLock>&& scheduler_lock) noexcept {
  CHECK_NE(fiberEntity, GetMasterFiberEntity()) << "Master fiber should not be added to run queue." ;

  fiberEntity->state_ = FiberState::READY;
  fiberEntity->sg_ = this;
  if (scheduler_lock) {
    scheduler_lock.unlock();
  }

  QueueRunnableEntity(fiberEntity);
}

void SchedulingGroup::Halt(
    FiberEntity* self, std::unique_lock<SpinLock>&& scheduler_lock) noexcept {
  CHECK_EQ(self, GetCurrentFiberEntity()) << "`self` must be pointer to caller's `FiberEntity`.";
  CHECK(scheduler_lock.owns_lock())  << "Scheduler lock must be held by caller prior to call to this method.";
  CHECK(self->state_ == FiberState::RUNNING) 
    << "`Halt()` is only for running fiber's use. If you want to `ReadyFiber()` "
      "yourself and `Halt()`, what you really need is `Yield()`.";
  
  auto master = GetMasterFiberEntity();
  self->state_ = FiberState::WAITING;

  master->OnResume(
      [self_lock = scheduler_lock.release()]() { self_lock->unlock(); });

  // When we're back, we should be in the same fiber.
  CHECK_EQ(self, GetCurrentFiberEntity());
}

void SchedulingGroup::Yield(FiberEntity* self) noexcept {
  auto master = GetMasterFiberEntity();

  master->state_ = FiberState::READY;
  SwitchTo(self, master);
}

void SchedulingGroup::SwitchTo(FiberEntity* self, FiberEntity* to) noexcept {
  CHECK_EQ(self, GetCurrentFiberEntity());

  CHECK(to->state_ == FiberState::READY) << "Fiber `to` is not in ready state.";
  CHECK_NE(self, to) <<  "Switch to yourself results in U.B.";

  to->OnResume([this, self]() {
    ReadyFiber(self, std::unique_lock(self->schedulerLock_));
  });

  // When we're back, we should be in the same fiber.
  CHECK_EQ(self, GetCurrentFiberEntity());
}

void SchedulingGroup::EnterGroup(std::size_t index) {
  CHECK(current_ == nullptr) << "This pthread worker has already joined a scheduling group.";
  CHECK(timerWorker_ != nullptr) << "The timer worker is not available yet.";
  CHECK_LT(index, 64) << "The worker number should < 64.";

  // Initialize thread-local timer queue for this worker.
  timerWorker_->InitializeLocalQueue(index);

  // Initialize scheduling-group information of this pthread.
  current_ = this;
  workerIndex_ = index;

  // Initialize master fiber for this worker.
  SetUpMasterFiberEntity();
}

void SchedulingGroup::LeaveGroup() {
  CHECK(current_ == this) << "This pthread worker does not belong to this scheduling group.";
  current_ = nullptr;
  workerIndex_ = kUninitializedWorkerIndex;
}

std::size_t SchedulingGroup::GroupSize() const noexcept { return groupSize_; }

void SchedulingGroup::SetTimerWorker(TimerWorker* worker) noexcept {
  timerWorker_ = worker;
}

void SchedulingGroup::Stop() {
  stopped_ = true;
  for (std::size_t index = 0; index != groupSize_; ++index) {
    waitSlots_[index].PersistentWake();
  }
}

bool SchedulingGroup::WakeUpOneWorker() noexcept {
  return WakeUpOneDeepSleepingWorker();
}

bool SchedulingGroup::WakeUpOneDeepSleepingWorker() noexcept {
  // We indeed have to wake someone that is in deep sleep then.
  while (true) {
    auto last_sleeping = __builtin_ffsll(sleepingWorkers_) - 1;
    if(last_sleeping < 0) {
      return true;
    }
    auto claiming_mask = 1ULL << last_sleeping;

    if (sleepingWorkers_.fetch_and(~claiming_mask) & claiming_mask) {
      CHECK_LT(last_sleeping, groupSize_);
      waitSlots_[last_sleeping].Wake();
      return true;
    }

    asm volatile(
        "pause" :
                :
                : 
        "memory"); 
  }
    return false;
}

void SchedulingGroup::QueueRunnableEntity(FiberEntity* entity) noexcept {
  std::scoped_lock lk(lock_);
  CHECK(!stopped_) << "The scheduling group has been stopped.";

  readyFiberQueue_.push(entity);

  WakeUpOneWorker();
}

} // namespace tinyRPC::fiber::detail