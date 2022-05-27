#ifndef _SRC_BASE_SCOPED_DEFERRED_H_
#define _SRC_BASE_SCOPED_DEFERRED_H_

#include <utility>
#include "Function.h"

namespace tinyRPC{

template<typename F>
class ScopedDeferred{
public:
    explicit ScopedDeferred(F&& f) : func_(std::move(f)){}
    // Nonmovable.
    ~ScopedDeferred() { func_(); }

    // Noncopyable.
    ScopedDeferred(const ScopedDeferred&) = delete;
    ScopedDeferred& operator=(const ScopedDeferred&) = delete;
private:
    F func_;
};

// Call action on destruction. Moveable. Dismissable.
class Deferred {
 public:
  Deferred() = default;

  template <class F>
  explicit Deferred(F&& f) : action_(std::forward<F>(f)) {}
  Deferred(Deferred&& other) noexcept { action_ = std::move(other.action_); }
  Deferred& operator=(Deferred&& other) noexcept {
    if (&other == this) {
      return *this;
    }
    Fire();
    action_ = std::move(other.action_);
    return *this;
  }
  ~Deferred() {
    if (action_) {
      action_();
    }
  }

  explicit operator bool() const noexcept { return !!action_; }

  void Fire() noexcept {
    if (auto op = std::move(action_)) {
      op();
    }
  }

  void Dismiss() noexcept { action_ = nullptr; }

 private:
  UniqueFunction<void()> action_;
};

}

#endif