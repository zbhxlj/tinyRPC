#ifndef _SRC_FIBER_DETAIL_WAITABLE_H_
#define _SRC_FIBER_DETAIL_WAITABLE_H_

#include <atomic>
#include <memory>
#include <chrono>
#include <mutex>

#include "FiberEntity.h"
#include "../../base/DoublyLinkedList.h"
#include "../../base/thread/SpinLock.h"
#include "glog/logging.h"

namespace tinyRPC::fiber::detail{

class SchedulingGroup;

// WaitBlock acts as `waiter` waiting on DoublyLinkedList.
struct WaitBlock{
    FiberEntity* waiter_ = nullptr;
    DoublyLinkedListEntry chain_;

    std::atomic<bool> satisfied_ = {false};
};

// `Waitable` acts as `waiting queue`.
// Thread-safe.
class Waitable {
 public:
  Waitable() = default;
  ~Waitable() { CHECK(waiters_.empty()); }

  // `FiberEntity::SchedulerLock` must be held in state transition
  // to prevent race condition.
  // In this method to prevent wake up loss.
  bool AddWaiter(WaitBlock* waiter);

  bool TryRemoveWaiter(WaitBlock* waiter);

  FiberEntity* WakeOne();

  // Set persistent awakened and return all pending waiters.
  std::vector<FiberEntity*> SetPersistentAwakened();

  // Undo `SetPersistentAwakened()`.
  void ResetAwakened();

  // Noncopyable, nonmovable.
  Waitable(const Waitable&) = delete;
  Waitable& operator=(const Waitable&) = delete;

 private:
  // Not lock free. 
  SpinLock lock_;
  bool persistentAwakened = false;
  DoublyLinkedList<WaitBlock, &WaitBlock::chain_> waiters_;
};

// `WaitableTimer` awakens all fibers when timeout. 
class WaitableTimer {
 public:
  explicit WaitableTimer(std::chrono::steady_clock::time_point expires_at);
  ~WaitableTimer();

  // Wait until the given time point is reached.
  void wait();

 private:
  static void OnTimerExpired(std::shared_ptr<Waitable> ref);

 private:
  SchedulingGroup* sg_;
  std::uint64_t timer_id_;

  std::shared_ptr<Waitable> impl_;
};

// `Mutex` for fiber.
class Mutex {
 public:
  bool try_lock() {
    CHECK(IsInFiberContext());

    std::uint32_t expected = 0;
    return count_.compare_exchange_strong(expected, 1);
  }

  void lock() {
    CHECK(IsInFiberContext());

    if (try_lock()) {
      return;
    }
    LockSlow();
  }

  void unlock();

 private:
  void LockSlow();

 private:
  Waitable impl_;

  // Synchronizes between slow path of `lock()` and `unlock()`.
  // To be specifically, make sure count_ and impl_(pending waiters)
  // is in consistent state.
  SpinLock slowLockPath_;

  // Number of waiters.
  std::atomic<std::uint32_t> count_{0};
};

// `ConditionalVariable` for fiber.
class ConditionVariable {
 public:
  void wait(std::unique_lock<Mutex>& lock);

  template <class F>
  void wait(std::unique_lock<Mutex>& lock, F&& pred) {
    CHECK(IsInFiberContext());

    while (!std::forward<F>(pred)()) {
      wait(lock);
    }

    CHECK(lock.owns_lock());
  }

  // You can always assume this method returns as a result of `notify_xxx` even
  // if it can actually results from timing out. This is conformant behavior --
  // it's just a spurious wake up in latter case.
  //
  // Returns `false` on timeout.
  bool wait_until(std::unique_lock<Mutex>& lock,
                  std::chrono::steady_clock::time_point expires_at);

  template <class F>
  bool wait_until(std::unique_lock<Mutex>& lk,
                  std::chrono::steady_clock::time_point timeout, F&& pred) {
    CHECK(IsInFiberContext());

    while (!std::forward<F>(pred)()) {
      wait_until(lk, timeout);
      if (std::chrono::steady_clock::now() >= timeout) {
        return std::forward<F>(pred)();
      }
    }
    CHECK(lk.owns_lock());
    return true;
  }

  void notify_one() noexcept;
  void notify_all() noexcept;

 private:
  Waitable impl_;
};

// `ExitBarrier` to implement `Fiber::Join()`.
class ExitBarrier {
 public:
  ExitBarrier();

  // Grab the lock required by `UnsafeCountDown()` in advance.
  std::unique_lock<Mutex> GrabLock();

  // Count down the barrier's internal counter and wake up waiters.
  void UnsafeCountDown(std::unique_lock<Mutex> lk);

  // Won't block.
  void Wait();

  void Reset() { count_ = 1; }

 private:
  Mutex lock_;
  std::size_t count_;
  ConditionVariable cv_;
};


} // namespace tinyRPC::fiber::detail

#endif