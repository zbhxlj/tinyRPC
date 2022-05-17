#include "Timer.h"
#include <chrono>
#include "Runtime.h"
#include "Fiber.h"

namespace tinyRPC::fiber {

detail::TimerPtr SetTimer(std::chrono::steady_clock::time_point at,
                       UniqueFunction<void()>&& cb) {
  auto sg = NearestSchedulingGroup();
  auto timer_id =
      sg->CreateTimer(at, [cb = std::move(cb)](auto timer_id) mutable {
        StartFiberDetached(
            [cb = std::move(cb)] { cb(); });
      });
  sg->EnableTimer(timer_id);
  return timer_id;               
}

detail::TimerPtr SetTimer(std::chrono::steady_clock::time_point at,
                       UniqueFunction<void(detail::TimerPtr& )>&& cb) {
  auto sg = NearestSchedulingGroup();
  auto timer_id =
      sg->CreateTimer(at, [cb = std::move(cb)](auto timer_id) mutable {
        StartFiberDetached(
            [cb = std::move(cb), timer_id]() mutable { cb(timer_id); });
      });
  sg->EnableTimer(timer_id);
  return timer_id;
}


detail::TimerPtr SetTimer(std::chrono::steady_clock::time_point at,
                       std::chrono::nanoseconds interval,
                       UniqueFunction<void()>&& cb) {
  struct UserCallback {
    void Run() {
      if (!running.exchange(true)) {
        cb();
      }
      running.store(false);
    }
    UniqueFunction<void()> cb;
    std::atomic<bool> running{};
  };

  auto ucb = std::make_shared<UserCallback>();
  ucb->cb = std::move(cb);

  auto sg = NearestSchedulingGroup();
  auto timer_id = sg->CreateTimer(at, interval, [ucb](auto timer_id) mutable {
    StartFiberDetached([ucb]() mutable { ucb->Run(); });
  });
  sg->EnableTimer(timer_id);
  return timer_id;
}

detail::TimerPtr SetTimer(std::chrono::steady_clock::time_point at,
                       std::chrono::nanoseconds interval,
                       UniqueFunction<void(detail::TimerPtr&)>&& cb) {
  struct UserCallback {
    void Run(detail::TimerPtr tid) {
      if (!running.exchange(true)) {
        cb(tid);
      }
      running.store(false);
    }
    UniqueFunction<void(detail::TimerPtr&)> cb;
    std::atomic<bool> running{};
  };

  auto ucb = std::make_shared<UserCallback>();
  ucb->cb = std::move(cb);

  auto sg = NearestSchedulingGroup();
  auto timer_id = sg->CreateTimer(at, interval, [ucb](auto tid) mutable {
    StartFiberDetached([ucb, tid]() mutable { ucb->Run(tid); });
  });
  sg->EnableTimer(timer_id);
  return timer_id;
}

detail::TimerPtr SetTimer(std::chrono::nanoseconds interval,
                       UniqueFunction<void()>&& cb) {
  return SetTimer(std::chrono::steady_clock::now() + interval, interval, std::move(cb));
}

detail::TimerPtr SetTimer(std::chrono::nanoseconds interval,
                       UniqueFunction<void(detail::TimerPtr&)>&& cb) {
  return SetTimer(std::chrono::steady_clock::now() + interval, interval, std::move(cb));
}


void KillTimer(detail::TimerPtr timer_id) {
  return detail::SchedulingGroup::GetTimerOwner(timer_id)->RemoveTimer(
      timer_id);
}

namespace internal {

[[nodiscard]] detail::TimerPtr CreateTimer(
    std::chrono::steady_clock::time_point at,
    UniqueFunction<void(detail::TimerPtr&)>&& cb) {
  return tinyRPC::fiber::NearestSchedulingGroup()->CreateTimer(at, std::move(cb));
}

[[nodiscard]] detail::TimerPtr CreateTimer(
    std::chrono::steady_clock::time_point at, std::chrono::nanoseconds interval,
    UniqueFunction<void(detail::TimerPtr&)>&& cb) {
  return tinyRPC::fiber::NearestSchedulingGroup()->CreateTimer(at, interval,
                                                       std::move(cb));
}

void EnableTimer(detail::TimerPtr& timer_id) {
  detail::SchedulingGroup::GetTimerOwner(timer_id)->EnableTimer(timer_id);
}

void KillTimer(detail::TimerPtr& timer_id) {
  return detail::SchedulingGroup::GetTimerOwner(timer_id)->RemoveTimer(
      timer_id);
}

}  // namespace internal
} // namespace tinyRPC::fiber