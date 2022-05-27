#ifndef _SRC_BASE_NEVER_DESTROYED_H_
#define _SRC_BASE_NEVER_DESTROYED_H_

#include <new>  // For placement new. @sa: https://stackoverflow.com/q/4010281
#include <type_traits>
#include <utility>

// Inspired by `wtf::NeverDestroyed`.
//
// @sa:
// https://github.com/WebKit/webkit/blob/master/Source/WTF/wtf/NeverDestroyed.h

namespace tinyRPC {

namespace detail {

template <class T>
class NeverDestroyedImpl {
 public:
  // Noncopyable / nonmovable.
  NeverDestroyedImpl(const NeverDestroyedImpl&) = delete;
  NeverDestroyedImpl& operator=(const NeverDestroyedImpl&) = delete;

  // Accessors.
  T* Get() noexcept { return reinterpret_cast<T*>(&storage_); }
  const T* Get() const noexcept {
    return reinterpret_cast<const T*>(&storage_);
  }

  T* operator->() noexcept { return Get(); }
  const T* operator->() const noexcept { return Get(); }

  T& operator*() noexcept { return *Get(); }
  const T& operator*() const noexcept { return *Get(); }

 protected:
  NeverDestroyedImpl() = default;

 protected:
  std::aligned_storage_t<sizeof(T), alignof(T)> storage_;
};

}  // namespace detail

// `NeverDestroyed<T>` helps you create objects that are never destroyed
// (without incuring heap memory allocation.).
//
// In certain cases (e.g., singleton), not destroying object can save you from
// dealing with destruction order issues.
//
// Caveats:
//
// - Be caution when declaring `NeverDestroyed<T>` as `thread_local`, this may
//   cause memory leak.
//
// - To construct `NeverDestroyed<T>`, you might have to declare this class as
//   your friend (if the constructor being called is not publicly accessible).
//
// - By declaring `NeverDestroyed<T>` as your friend, it's impossible to
//   guarantee `T` is used as a singleton as now anybody can construct a new
//   `NeverDestroyed<T>`. You can use `NeverDestroyedSingleton<T>` in this case.
//
// e.g.:
//
// void CreateWorld() {
//   static NeverDestroyed<std::mutex> lock;  // Destructor won't be called.
//   std::scoped_lock _(*lock);
//
//   // ...
// }
template <class T>
class NeverDestroyed final : private detail::NeverDestroyedImpl<T> {
  using Impl = detail::NeverDestroyedImpl<T>;

 public:
  template <class... Ts>
  explicit NeverDestroyed(Ts&&... args) {
    new (&this->storage_) T(std::forward<Ts>(args)...);
  }

  using Impl::Get;
  using Impl::operator->;
  using Impl::operator*;
};

// Same as `NeverDestroyed`, except that it's constructor is only accessible to
// `T`. This class is useful when `T` is intended to be used as singleton.
template <class T>
class NeverDestroyedSingleton final
    : private tinyRPC::detail::NeverDestroyedImpl<T> {
  using Impl = detail::NeverDestroyedImpl<T>;

 public:
  using Impl::Get;
  using Impl::operator->;
  using Impl::operator*;

 private:
  friend T;

  template <class... Ts>
  explicit NeverDestroyedSingleton(Ts&&... args) {
    new (&this->storage_) T(std::forward<Ts>(args)...);
  }
};

}  // namespace tinyRPC

#endif  
