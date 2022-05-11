#include "Waitable.h"
#include <memory>
#include <mutex>
#include "FiberEntity.h"

namespace tinyRPC::fiber::detail{

bool Waitable::AddWaiter(WaitBlock *waiter){
    std::scoped_lock _(lock_);

    CHECK(waiter->waiter_);
    if(persistentAwakened){
        return false;
    }
    waiters_.push_back(waiter);
    return true;
}

bool Waitable::TryRemoveWaiter(WaitBlock *waiter){
    std::scoped_lock _(lock_);
    return waiters_.erase(waiter);
}

FiberEntity* Waitable::WakeOne() {
  std::scoped_lock _(lock_);
  while (true) {
    auto waiter = waiters_.pop_front();
    if (!waiter) {
      return nullptr;
    }
    if (waiter->satisfied_.exchange(true)) {
      continue;  // It's awakened by someone else.
    }
    return waiter->waiter_;
  }
}

std::vector<FiberEntity*> Waitable::SetPersistentAwakened() {
  std::scoped_lock _(lock_);
  persistentAwakened = true;

  std::vector<FiberEntity*> wbs;
  while (auto ptr = waiters_.pop_front()) {
    if (ptr->satisfied_.exchange(true)) {
      continue;
    }
    wbs.push_back(ptr->waiter_);
  }
  return wbs;
}

void Waitable::ResetAwakened() {
  std::scoped_lock _(lock_);
  persistentAwakened = false;
}


WaitableTimer::WaitableTimer(std::chrono::steady_clock::time_point expires_at)
    // TODO: implement `SchedulingGroup`
    : sg_(SchedulingGroup::Current()),
      impl_(std::make_shared<Waitable>()) {
  timer_id_ = sg_->CreateTimer(
      expires_at, [ref = impl_]() { OnTimerExpired(std::move(ref)); });
  sg_->EnableTimer(timer_id_);
}

WaitableTimer::~WaitableTimer() { sg_->RemoveTimer(timer_id_); }

void WaitableTimer::wait() {
  CHECK(IsInFiberContext());

  auto current = GetCurrentFiberEntity();
  WaitBlock wb = {.waiter_ = current};
  std::unique_lock lk(current->schedulerLock_);
  if (impl_->AddWaiter(&wb)) {
    current->sg_->Halt(current, std::move(lk));
    // We'll be awakened by `OnTimerExpired()`.
  } else {
    // The timer has fired before, return immediately then.
    // Already set persistentAwakened.
  }
}

void WaitableTimer::OnTimerExpired(std::shared_ptr<Waitable> ref) {
  auto fibers = ref->SetPersistentAwakened();
  for (auto f : fibers) {
    f->sg_->ReadyFiber(f, std::unique_lock(f->scheduler_lock));
  }
}

void Mutex::unlock() {
  CHECK(IsInFiberContext());

  if (auto was = count_.fetch_sub(1); was == 1) {
  } else {
    CHECK_GT(was, 1);

    std::unique_lock splk(slowLockPath_);
    auto fiber = impl_.WakeOne();
    CHECK(fiber);  // Otherwise `was` must be 1 (as there's no waiter).
    splk.unlock();
    fiber->sg_->ReadyFiber(
        fiber, std::unique_lock(fiber->scheduler_lock));
  }
}

void Mutex::LockSlow() {
  CHECK(IsInFiberContext());

  std::unique_lock splk(slowLockPath_);

  if (count_.fetch_add(1, std::memory_order_acquire) == 0) {
    // The owner released the lock before we incremented `count_`.
    return;
  }

  auto current = GetCurrentFiberEntity();
  WaitBlock wb = {.waiter_ = current};
  CHECK(impl_.AddWaiter(&wb));  

  // Now we halt ourselves. Hold fiber's `SchedulerLock` to prevent
  // anyone else from changing fiber's state. (Awake us before we call `Halt()` i.e.)
  std::unique_lock slk(current->schedulerLock_);
  // Now the slow path lock can be unlocked.
  //
  // Indeed it's possible that we're awakened even before we call `Halt()`,
  // but this issue is already addressed by `scheduler_lock` (which we're
  // holding).
  splk.unlock();

  // Wait until we're woken by `unlock()`.
  //
  // Given that `scheduler_lock` is held by us, anyone else who concurrently
  // tries to wake us up is blocking on it until `Halt()` has completed.
  // Hence no race here.
  current->sg_->Halt(current, std::move(slk));

  // Lock's owner has awakened us up, the lock is in our hand then.
  CHECK(!impl_.TryRemoveWaiter(&wb));
  return;
}

// Utility for waking up a fiber sleeping on a `Waitable` asynchronously.
class AsyncWaker {
 public:
  AsyncWaker(SchedulingGroup* sg, FiberEntity* self, WaitBlock* wb)
      : sg_(sg), self_(self), wb_(wb) {}

  // The destructor does some sanity checks.
  ~AsyncWaker() { CHECK_EQ(timer_, 0) <<"Have you called `Cleanup()`?"; }

  // Set a timer to awake `self` once `expires_at` is reached.
  void SetTimer(std::chrono::steady_clock::time_point expires_at) {
    wait_cb_ = std::make_shared<WaitCb>();
    wait_cb_->waiter = self_;

    // This callback wakes us up if we times out.
    auto timer_cb = [wait_cb = wait_cb_ /* ref-counted */, wb = wb_](auto) {
      std::scoped_lock lk(wait_cb->lock);
      if (!wait_cb->awake) {  // It's (possibly) timed out.
        // We're holding the lock, and `wait_cb->awake` has not been set yet, so
        // `Cleanup()` cannot possibly finished yet. Therefore, we can be sure
        // `wb` is still alive.
        if (wb->satisfied_.exchange(true)) {
          // Someone else satisfied the wait earlier.
          // This happens between wake up from `notify_xxx()` and call `Cleanup()` .
          return;
        }
        wait_cb->waiter->sg_->ReadyFiber(
            wait_cb->waiter, std::unique_lock(wait_cb->waiter->scheduler_lock));
      }
    };

    // Set the timeout timer.
    timer_ = sg_->CreateTimer(expires_at, timer_cb);
    sg_->EnableTimer(timer_);
  }

  // Prevent the timer set by this class from waking up `self` again.
  void Cleanup() {
    // If `timer_cb` has returned, nothing special; if `timer_cb` has never
    // started, nothing special. But if `timer_cb` is running, we need to
    // prevent it from `ReadyFiber` us again (when we immediately sleep on
    // another unrelated thing.).
    sg_->RemoveTimer(std::exchange(timer_, 0));
    {
      // Here is the trick.
      //
      // We're running now, therefore our `WaitBlock::satisfied` has been set.
      // Our `timer_cb` will check the flag, and bail out without waking us
      // again.
      std::scoped_lock _(wait_cb_->lock);
      wait_cb_->awake = true;
    }  // `wait_cb_->awake` has been set, so other fields of us won't be touched
       // by `timer_cb`. we're safe to destruct from now on.
  } 

 private:
  // Ref counted as it's used both by us and an asynchronous timer.
  struct WaitCb {
    SpinLock lock;
    FiberEntity* waiter;
    bool awake = false;
  };

  SchedulingGroup* sg_;
  FiberEntity* self_;
  WaitBlock* wb_;
  std::shared_ptr<WaitCb> wait_cb_;
  std::uint64_t timer_ = 0;
};


void ConditionVariable::wait(std::unique_lock<Mutex>& lock) {
  CHECK(IsInFiberContext());
  CHECK(lock.owns_lock());

  wait_until(lock, std::chrono::steady_clock::time_point::max());
}

bool ConditionVariable::wait_until(
    std::unique_lock<Mutex>& lock,
    std::chrono::steady_clock::time_point expires_at) {
  CHECK(IsInFiberContext());

  auto current = GetCurrentFiberEntity();
  auto sg = current->sg_;
  bool use_timeout = expires_at != std::chrono::steady_clock::time_point::max();

  // Add us to the wait queue.
  std::unique_lock slk(current->scheduler_lock);
  WaitBlock wb = {.waiter_ = current};
  CHECK(impl_.AddWaiter(&wb));
  AsyncWaker awaker(sg, current, &wb);
  if (use_timeout) {  // Set a timeout if needed.
    awaker.SetTimer(expires_at);
  }

  // Release user's lock.
  lock.unlock();

  // Block until being waken up by either `notify_xxx` or the timer.
  sg->Halt(current, std::move(slk));  // `slk` is released by `Halt()`.

  // Try remove us from the wait chain. This operation will fail if we're
  // awakened by `notify_xxx()`.
  auto timeout = impl_.TryRemoveWaiter(&wb);

  if (use_timeout) {
    // Stop the timer we've set.
    awaker.Cleanup();
  }

  // Grab the lock again and return.
  lock.lock();
  return !timeout;
}

void ConditionVariable::notify_one() noexcept {
  CHECK(IsInFiberContext());

  auto fiber = impl_.WakeOne();
  if (!fiber) {
    return;
  }
  fiber->sg_->ReadyFiber(fiber, std::unique_lock(fiber->scheduler_lock));
}

void ConditionVariable::notify_all() noexcept {
  CHECK(IsInFiberContext());

  // Think of how you use it.
  // We must move all fibers out and then schedule them.
  // If you call `notify_one()` in a loop, it is likely
  // some will immediately wait again and be notified again. 
  std::vector<FiberEntity*> fibers;

  while (true) {
    auto fiber = impl_.WakeOne();
    if (!fiber) {
      break;
    }
      fibers.push_back(fiber);
  }

  // Schedule the waiters.
  for (auto&& e : fibers) {
    e->sg_->ReadyFiber(e, std::unique_lock(e->scheduler_lock));
  }
}

ExitBarrier::ExitBarrier() : count_(1) {}

std::unique_lock<Mutex> ExitBarrier::GrabLock() {
  CHECK(IsInFiberContext());
  return std::unique_lock(lock_);
}

void ExitBarrier::UnsafeCountDown(std::unique_lock<Mutex> lk) {
  CHECK(IsInFiberContext());
  CHECK(lk.owns_lock() && lk.mutex() == &lock_);

  CHECK_GT(count_, 0);
  if (!--count_) {
    cv_.notify_all();
  }
}

void ExitBarrier::Wait() {
  CHECK(IsInFiberContext());

  std::unique_lock lk(lock_);
  return cv_.wait(lk, [this] { return count_ == 0; });
}

} // namespace tinyRPC::fiber::detail