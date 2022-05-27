#ifndef _SRC_BASE_CALLBACK_H_
#define _SRC_BASE_CALLBACK_H_

#include <utility>

#include "google/protobuf/service.h"  // ...

// You shouldn't be using `google::protobuf::Closure`..
//
// @sa: https://github.com/protocolbuffers/protobuf/issues/1505

namespace tinyRPC {

namespace detail {

template <class Impl, bool kSelfDestroying>
class Callback : public google::protobuf::Closure {
 public:
  template <class T>
  explicit Callback(T&& impl) : impl_(std::forward<T>(impl)) {}

  void Run() override {
    impl_();
    if (kSelfDestroying) {
      delete this;
    }
  }

 private:
  Impl impl_;
};

}  // namespace detail

namespace internal {

template <class T>
class LocalCallback : public google::protobuf::Closure {
 public:
  explicit LocalCallback(T&& impl) : impl_(std::forward<T>(impl)) {}

  void Run() override { impl_(); }

 private:
  T impl_;
};

}  // namespace internal

template <class F>
google::protobuf::Closure* NewCallback(F&& f) {
  return new detail::Callback<std::remove_reference_t<F>, true>(
      std::forward<F>(f));
}

template <class F>
google::protobuf::Closure* NewPermanentCallback(F&& f) {
  return new detail::Callback<std::remove_reference_t<F>, false>(
      std::forward<F>(f));
}

}  // namespace tinyRPC

#endif  
