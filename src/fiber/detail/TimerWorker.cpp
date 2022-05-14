#include <chrono>
#include <memory>

#include "../../../include/glog/logging.h"
#include "../../base/base.h"
#include "TimerWorker.h"
#include "SchedulingGroup.h"

namespace tinyRPC::fiber::detail{

thread_local bool threadLocalQueueInitialized = false;

ThreadLocalQueue* TimerWorker::GetThreadLocalQueue() {
  thread_local ThreadLocalQueue q;
  return &q;
}

// To avoid overflow in libstdc++.
std::chrono::steady_clock::time_point GetSleepTimeout(
    std::chrono::steady_clock::duration expected) {
  if (expected == std::chrono::steady_clock::duration::max()) {
    return std::chrono::steady_clock::now() + std::chrono::hours(24);  // Randomly chosen.
  } else {
    return std::chrono::steady_clock::time_point(expected);
  }
}

TimerWorker::TimerWorker(SchedulingGroup* sg)
    : sg_(sg), localQueues_(sg->GroupSize() + 1), latch_(sg->GroupSize() + 1) {}

TimerWorker::~TimerWorker() = default;

TimerWorker* TimerWorker::GetTimerOwner(TimerPtr& timer) {
  return timer->owner_;
}

TimerPtr TimerWorker::CreateTimer(
    std::chrono::steady_clock::time_point expires_at,
    std::function<void(TimerPtr&)>&& cb) {
  CHECK(cb) << "No callback for the timer?";

  auto timer = std::make_shared<Timer>();
  timer->owner_ = this;
  timer->cancelled_ = false;
  timer->cb_ = std::move(cb);
  timer->expiresAt_ = expires_at;
  timer->periodic_ = false;

  CHECK_EQ(timer.use_count(), 1);
  return timer;
}

TimerPtr TimerWorker::CreateTimer(
    std::chrono::steady_clock::time_point initial_expires_at,
    std::chrono::nanoseconds interval, std::function<void(TimerPtr&)>&& cb) {
  CHECK(cb) << "No callback for the timer?";
  CHECK_LT(interval, static_cast<std::chrono::nanoseconds>(0ull)) <<
              "`interval` must be greater than 0 for periodic timers.";
  if (std::chrono::steady_clock::now() > 
            initial_expires_at + static_cast<std::chrono::seconds>(10ull)) {
    LOG(ERROR) << "`initial_expires_at` was specified as long ago. Corrected to now.";
    initial_expires_at = std::chrono::steady_clock::now();
  }

  auto timer = std::make_shared<Timer>();
  timer->owner_ = this;
  timer->cancelled_ = false;
  timer->cb_ = std::move(cb);
  timer->expiresAt_ = initial_expires_at;
  timer->periodic_ = true;
  timer->interval_ = interval;

  CHECK_EQ(timer.use_count(), 1);
  return timer;
}

void TimerWorker::EnableTimer(TimerPtr& timer) {
  AddTimer(timer);
}

void TimerWorker::RemoveTimer(TimerPtr& timer) {
  CHECK_EQ(timer->owner_, this) <<
                 "The timer you're trying to detach does not belong to this "
                 "scheduling group.";
  std::function<void(TimerPtr&)> cb;
  {
    std::scoped_lock _(timer->lock_);
    timer->cancelled_.store(true);
    cb = std::move(timer->cb_);
  }
}

SchedulingGroup* TimerWorker::GetSchedulingGroup() { return sg_; }

void TimerWorker::InitializeLocalQueue(std::size_t worker_index) {
  if (worker_index == SchedulingGroup::kTimerWorkerIndex) {
    worker_index = sg_->GroupSize();
  }
  CHECK_LT(worker_index, sg_->GroupSize() + 1);
  CHECK(localQueues_[worker_index] == nullptr)
              << "Someone else has registered itself as worker "<< worker_index << std::endl;
  localQueues_[worker_index] = GetThreadLocalQueue();
  threadLocalQueueInitialized = true;
  latch_.count_down();
}

void TimerWorker::Start() {
  worker_ = std::thread([&] {
    SetCurrentThreadName("TimerWorker");
    WorkerProc();
  });
}

void TimerWorker::Stop() {
  std::scoped_lock _(lock_);
  stopped_.store(true);
  cv_.notify_one();
}

void TimerWorker::Join() { worker_.join(); }

void TimerWorker::WorkerProc() {
  sg_->EnterGroup(SchedulingGroup::kTimerWorkerIndex);
  WaitForWorkers();  // Wait for other workers to come in.

  while (!stopped_) {
    // TODO: Figure out why we need reset nextExpireAt to max().
    // Collect thread-local timer queues into our central heap.
    ReapThreadLocalQueues();

    // And fire those who has expired.
    FireTimers();

    // TODO: Why wake again?

    // Sleep until next time fires.
    std::unique_lock lk(lock_);
    auto expected = nextExpireAt;
    cv_.wait_until(lk, GetSleepTimeout(expected), [&] {
      return nextExpireAt != expected || stopped_;
    });
  }
  sg_->LeaveGroup();
}

void TimerWorker::AddTimer(TimerPtr timer) {
  CHECK(threadLocalQueueInitialized) << "You must initialize your thread-local queue (done as part of "
              "`SchedulingGroup::EnterGroup()` before calling `AddTimer`." << std::endl;
  CHECK_EQ(timer.use_count(), 2);  // One is caller, one is us.

  auto&& tls_queue = GetThreadLocalQueue();
  std::unique_lock lk(tls_queue->lock_);  // This is cheap (relatively, I mean).
  auto expires_at = timer->expiresAt_;
  tls_queue->timers_.push_back(std::move(timer));

  if (tls_queue->expiresAt_ > expires_at) {
    tls_queue->expiresAt_ = expires_at;
    lk.unlock();
    WakeWorkerIfNeeded(expires_at);
  }
}

void TimerWorker::WaitForWorkers() { latch_.wait(); }

void TimerWorker::ReapThreadLocalQueues() {
  for (auto&& p : localQueues_) {
    std::vector<TimerPtr> t;
    {
      std::scoped_lock _(p->lock_);
      t.swap(p->timers_);
      // Reset.
      p->expiresAt_ = std::chrono::steady_clock::time_point::max();
    }
    for (auto&& e : t) {
      if (e->cancelled_ == true) {
        continue;
      }
      timers_.push(std::move(e));
    }
  }
}

void TimerWorker::FireTimers() {
  auto now = std::chrono::steady_clock::now();
  while (!timers_.empty()) {
    TimerPtr& e =  const_cast<TimerPtr&>(timers_.top());
    if (e->cancelled_ == true) {
      timers_.pop();
      continue;
    }
    if (e->expiresAt_ > now) {
      break;
    }

    std::unique_lock lk(e->lock_);
    auto cb = std::move(e->cb_);
    lk.unlock();
    
    CHECK(cb) << "No callback set" << std::endl;
    cb(e);

    // If it's a periodic timer, add a new pending timer.
    if (e->periodic_) {
        std::unique_lock lk(e->lock_);
        if (!e->cancelled_) {
            e->expiresAt_ = e->expiresAt_ + e->interval_;
            e->cb_ = std::move(cb);  // Move user's callback back.
            lk.unlock();
            timers_.push(std::move(e));
        }
    }
    timers_.pop();
  }
}

void TimerWorker::WakeWorkerIfNeeded(
    std::chrono::steady_clock::time_point local_expires_at) {
  auto expires_at = local_expires_at.time_since_epoch();

  while (true) {
    if (nextExpireAt <= expires_at) {  // Nothing to do then.
      return;
    }

    std::scoped_lock _(lock_);
    nextExpireAt = expires_at;
    cv_.notify_one();
    return;
  }
}

} // namespace tinyRPC::fiber::detail