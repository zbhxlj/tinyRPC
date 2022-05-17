#ifndef _SRC_FIBER_LATCH_H_
#define _SRC_FIBER_LATCH_H_

#include <chrono>
#include <mutex>

#include "../../include/glog/logging.h"
#include "ConditionalVariable.h"
#include "Mutex.h"

namespace tinyRPC::fiber{

class Latch {
 public:
  explicit Latch(std::ptrdiff_t count);

  // moment.
  void count_down(std::ptrdiff_t update = 1);

  bool try_wait() const noexcept;

  void wait() const;

  template <class Rep, class Period>
  bool wait_for(std::chrono::duration<Rep, Period> timeout) {
    std::unique_lock lk(lock_);
    CHECK_GE(count_, 0);
    return cv_.wait_for(lk, timeout, [this] { return count_ == 0; });
  }

  template <class Clock, class Duration>
  bool wait_until(std::chrono::time_point<Clock, Duration> timeout) {
    std::unique_lock lk(lock_);
    CHECK_GE(count_, 0);
    return cv_.wait_until(lk, timeout, [this] { return count_ == 0; });
  }

  void arrive_and_wait(std::ptrdiff_t update = 1);

 private:
  mutable Mutex lock_;
  mutable ConditionVariable cv_;
  std::ptrdiff_t count_;
};

} // namespace tinyRPC::fiber

#endif
