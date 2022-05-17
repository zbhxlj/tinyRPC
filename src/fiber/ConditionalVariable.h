#ifndef _SRC_FIBER_CONDITIONALVARIABLE_H_
#define _SRC_FIBER_CONDITIONALVARIABLE_H_

#include <chrono>
#include "Mutex.h"
#include "detail/Waitable.h"

namespace tinyRPC::fiber{

class ConditionVariable {
 public:
  void notify_one() noexcept;

  void notify_all() noexcept;

  void wait(std::unique_lock<Mutex>& lock);

  template <class Predicate>
  void wait(std::unique_lock<Mutex>& lock, Predicate pred);

  template <class Rep, class Period>
  bool wait_for(std::unique_lock<Mutex>& lock,
                          std::chrono::duration<Rep, Period> expires_in);

  template <class Rep, class Period, class Predicate>
  bool wait_for(std::unique_lock<Mutex>& lock,
                std::chrono::duration<Rep, Period> expires_in, Predicate pred);

  template <class Clock, class Duration>
  bool wait_until(
      std::unique_lock<Mutex>& lock,
      std::chrono::time_point<Clock, Duration> expires_at);

  template <class Clock, class Duration, class Pred>
  bool wait_until(std::unique_lock<Mutex>& lock,
                  std::chrono::time_point<Clock, Duration> expires_at,
                  Pred pred);

 private:
  detail::ConditionVariable impl_;
};

template <class Predicate>
void ConditionVariable::wait(std::unique_lock<Mutex>& lock, Predicate pred) {
  impl_.wait(lock, pred);
}

template <class Rep, class Period>
bool ConditionVariable::wait_for(
    std::unique_lock<Mutex>& lock,
    std::chrono::duration<Rep, Period> expires_in) {
  auto steady_timeout = std::chrono::steady_clock::now() + expires_in;
  return impl_.wait_until(lock, steady_timeout);
}

template <class Rep, class Period, class Predicate>
bool ConditionVariable::wait_for(std::unique_lock<Mutex>& lock,
                                 std::chrono::duration<Rep, Period> expires_in,
                                 Predicate pred) {
  auto steady_timeout = std::chrono::steady_clock::now() + expires_in;
  return impl_.wait_until(lock, steady_timeout, pred);
}

template <class Clock, class Duration>
bool ConditionVariable::wait_until(
    std::unique_lock<Mutex>& lock,
    std::chrono::time_point<Clock, Duration> expires_at) {
  auto steady_timeout = std::chrono::steady_clock::now() + (expires_at - Clock::now());
  return impl_.wait_until(lock, steady_timeout);
}

template <class Clock, class Duration, class Pred>
bool ConditionVariable::wait_until(
    std::unique_lock<Mutex>& lock,
    std::chrono::time_point<Clock, Duration> expires_at, Pred pred) {
  auto steady_timeout = std::chrono::steady_clock::now() + (expires_at - Clock::now());
  return impl_.wait_until(lock, steady_timeout, pred);
}

} // namespace tinyRPC::fiber

#endif