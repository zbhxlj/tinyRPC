#include "Latch.h"

namespace tinyRPC {

Latch::Latch(std::ptrdiff_t count) : count_(count) {}

void Latch::count_down(std::ptrdiff_t update) {
  std::unique_lock lk(m_);
  FLARE_CHECK_GE(count_, update);
  count_ -= update;
  if (!count_) {
    cv_.notify_all();
  }
}

bool Latch::try_wait() const noexcept {
  std::scoped_lock _(m_);
  FLARE_CHECK_GE(count_, 0);
  return !count_;
}

void Latch::wait() const {
  std::unique_lock lk(m_);
  FLARE_CHECK_GE(count_, 0);
  return cv_.wait(lk, [this] { return count_ == 0; });
}

void Latch::arrive_and_wait(std::ptrdiff_t update) {
  count_down(update);
  wait();
}

}  // namespace tinyRPC
