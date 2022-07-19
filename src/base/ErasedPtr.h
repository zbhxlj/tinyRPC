#ifndef _SRC_BASE_ERASEDPTR_H_
#define _SRC_BASE_ERASEDPTR_H_

#include <utility>

// Copy from flare.
// RAII wrapper for holding type erased pointers.

namespace tinyRPC{
// RAII wrapper for holding type erased pointers. Type-safety is your own
// responsibility.
class ErasedPtr final {
 public:
  using Deleter = void (*)(void*);

  // A default constructed one is an empty one.
  /* implicit */ constexpr ErasedPtr(std::nullptr_t = nullptr)
      : ptr_(nullptr), deleter_(nullptr) {}

  // Ownership taken.
  template <class T>
  constexpr explicit ErasedPtr(T* ptr) noexcept
      : ptr_(ptr), deleter_([](void* ptr) { delete static_cast<T*>(ptr); }) {}
  template <class T, class D>
  constexpr ErasedPtr(T* ptr, D deleter) noexcept
      : ptr_(ptr), deleter_(deleter) {}

  // Movable
  constexpr ErasedPtr(ErasedPtr&& ptr) noexcept
      : ptr_(ptr.ptr_), deleter_(ptr.deleter_) {
    ptr.ptr_ = nullptr;
  }

  ErasedPtr& operator=(ErasedPtr&& ptr) noexcept {
    if (&ptr != this) {
      Reset();
    }
    std::swap(ptr_, ptr.ptr_);
    std::swap(deleter_, ptr.deleter_);
    return *this;
  }

  // But not copyable.
  ErasedPtr(const ErasedPtr&) = delete;
  ErasedPtr& operator=(const ErasedPtr&) = delete;

  // Any resource we're holding is freed in dtor.
  ~ErasedPtr() {
    if (ptr_) {
      deleter_(ptr_);
    }
  }

  // Accessor.
  constexpr void* Get() const noexcept { return ptr_; }

  // It's your responsibility to check if the type match.
  template <class T>
  T* UncheckedGet() const noexcept {
    return reinterpret_cast<T*>(Get());
  }

  // Test if this object holds a valid pointer.
  constexpr explicit operator bool() const noexcept { return !!ptr_; }

  // Free any resource this class holds and reset its internal pointer to
  // `nullptr`.
  constexpr void Reset(std::nullptr_t = nullptr) noexcept {
    if (ptr_) {
      deleter_(ptr_);
      ptr_ = nullptr;
    }
  }

  // Release ownership of its internal object.
  [[nodiscard]] void* Leak() noexcept { return std::exchange(ptr_, nullptr); }

  // This is the only way you can destroy the pointer you obtain from `Leak()`.
  constexpr Deleter GetDeleter() const noexcept { return deleter_; }

 private:
  void* ptr_;
  Deleter deleter_;
};

template <class T, class... Args>
ErasedPtr MakeErased(Args&&... args) {
  return ErasedPtr(new T(std::forward<Args>(args)...));
}
}

#endif