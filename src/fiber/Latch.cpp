#include "Latch.h"

namespace tinyRPC::fiber {

Latch::Latch(std::ptrdiff_t count) : count_(count) {}

void Latch::count_down(std::ptrdiff_t update) {
  std::scoped_lock _(lock_);
  CHECK_GE(count_, update);
  count_ -= update;
  if (!count_) {
    cv_.notify_all();
  }
}

bool Latch::try_wait() const noexcept {
  std::scoped_lock _(lock_);
  CHECK_GE(count_, 0);
  return !count_;
}

void Latch::wait() const {
  std::unique_lock lk(lock_);
  CHECK_GE(count_, 0);
  cv_.wait(lk, [&] { return count_ == 0; });
}

void Latch::arrive_and_wait(std::ptrdiff_t update) {
  count_down(update);
  wait();
}

}  // namespace tinyRPC::fiber
