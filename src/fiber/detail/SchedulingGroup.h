#ifndef _SRC_FIBER_DETAIL_SCHEDULING_GROUP_H_
#define _SRC_FIBER_DETAIL_SCHEDULING_GROUP_H_

#include <cstdint>
#include <functional>
#include <queue>

#include "../../../glog/logging.h"
#include "TimerWorker.h"
#include "../../base/thread/SpinLock.h"

namespace tinyRPC::fiber::detail{

class FiberEntity;
class Timer;

class SchedulingGroup {
 public:
  inline static FiberEntity* const kSchedulingGroupShuttingDown =
      reinterpret_cast<FiberEntity*>(0x1);

  inline static constexpr std::size_t kTimerWorkerIndex = -1;

  explicit SchedulingGroup(std::size_t size);

  ~SchedulingGroup();

  static SchedulingGroup* Current() noexcept { return current_; }

  static SchedulingGroup* GetTimerOwner(TimerPtr& timer) noexcept {
    return TimerWorker::GetTimerOwner(timer)->GetSchedulingGroup();
  }

  FiberEntity* AcquireFiber() noexcept;

  FiberEntity* WaitForFiber() noexcept;

  FiberEntity* RemoteAcquireFiber() noexcept;

  void StartFiber(FiberEntity* fiberEntity) noexcept;

  void ReadyFiber(FiberEntity* fiber,
                  std::unique_lock<SpinLock>&& scheduler_lock) noexcept;

  void Halt(FiberEntity* self,
            std::unique_lock<SpinLock>&& scheduler_lock) noexcept;

  void Yield(FiberEntity* self) noexcept;

  void SwitchTo(FiberEntity* self, FiberEntity* to) noexcept;

  [[nodiscard]] TimerPtr CreateTimer(
      std::chrono::steady_clock::time_point expires_at,
    std::function<void(TimerPtr& )>&& cb) {
    CHECK(timerWorker_);
    CHECK_EQ(Current(), this);
    return timerWorker_->CreateTimer(expires_at, std::move(cb));
  }

  // Periodic timer.
  [[nodiscard]] TimerPtr CreateTimer(
      std::chrono::steady_clock::time_point initial_expires_at,
      std::chrono::nanoseconds interval, std::function<void(TimerPtr& )>&& cb) {
    CHECK(timerWorker_);
    CHECK_EQ(Current(), this);
    return timerWorker_->CreateTimer(initial_expires_at, interval,
                                      std::move(cb));
  }

  void EnableTimer(TimerPtr& timer) {
    CHECK(timerWorker_);
    CHECK_EQ(Current(), this);
    return timerWorker_->EnableTimer(timer);
  }

//   void DetachTimer(std::uint64_t timer_id) noexcept {
//     FLARE_CHECK(timer_worker_);
//     return timer_worker_->DetachTimer(timer_id);
//   }


  void RemoveTimer(TimerPtr& timer) noexcept {
    CHECK(timerWorker_);
    return timerWorker_->RemoveTimer(timer);
  }

  void EnterGroup(std::size_t index);

  void LeaveGroup();

  std::size_t GroupSize() const noexcept;


  void SetTimerWorker(TimerWorker* worker) noexcept;

  void Stop();

 private:
  bool WakeUpOneWorker() noexcept;
  
  bool WakeUpOneDeepSleepingWorker() noexcept;

  void QueueRunnableEntity(FiberEntity* fiberEntity) noexcept;

 private:
  static constexpr auto kUninitializedWorkerIndex =
      std::numeric_limits<std::size_t>::max();
  static thread_local SchedulingGroup* current_;
  static thread_local std::size_t workerIndex_;

  class WaitSlot;

  std::atomic<bool> stopped_{false};
  std::size_t groupSize_;
  TimerWorker* timerWorker_ = nullptr;

  SpinLock lock_;

  std::queue<FiberEntity*> readyFiberQueue_;

  // Fiber workers sleep on this.
  std::unique_ptr<WaitSlot[]> waitSlots_;

  std::atomic<std::uint64_t> sleepingWorkers_{0};

};

}

#endif