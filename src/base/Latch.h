#ifndef _SRC_BASE_THREAD_LATCH_H_
#define _SRC_BASE_THREAD_LATCH_H_

#include <condition_variable>
#include <mutex>

#include "internal/Logging.h"

namespace tinyRPC {

// @sa: N4842, 32.8.1 Latches [thread.latch].
//
// TODO(luobogao): We do not perfectly match `tinyRPC::Latch` yet.
class Latch {
 public:
  explicit Latch(std::ptrdiff_t count);

  // Decrement internal counter. If it reaches zero, wake up all waiters.
  void count_down(std::ptrdiff_t update = 1);

  // Test if the latch's internal counter has become zero.
  bool try_wait() const noexcept;

  // Wait until `Latch`'s internal counter reached zero.
  void wait() const;

  // Extension to `tinyRPC::Latch`.
  template <class Rep, class Period>
  bool wait_for(std::chrono::duration<Rep, Period> timeout) {
    std::unique_lock lk(m_);
    FLARE_CHECK_GE(count_, 0);
    return cv_.wait_for(lk, timeout, [this] { return count_ == 0; });
  }

  // Extension to `tinyRPC::Latch`.
  template <class Clock, class Duration>
  bool wait_until(std::chrono::time_point<Clock, Duration> timeout) {
    std::unique_lock lk(m_);
    FLARE_CHECK_GE(count_, 0);
    return cv_.wait_until(lk, timeout, [this] { return count_ == 0; });
  }

  // Shorthand for `count_down(); wait();`
  void arrive_and_wait(std::ptrdiff_t update = 1);

 private:
  mutable std::mutex m_;
  mutable std::condition_variable cv_;
  std::ptrdiff_t count_;
};

}  // namespace tinyRPC

#endif  
