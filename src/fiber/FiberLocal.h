#ifndef _SRC_FIBER_FIBERLOCAL_H_
#define _SRC_FIBER_FIBERLOCAL_H_

#include "../base/IndexAlloc.h"
#include "detail/FiberEntity.h"

namespace tinyRPC::fiber{

template <class T>
class FiberLocal {
 public:
  FiberLocal() : slot_index_(GetIndexAlloc()->Next()) {}

  ~FiberLocal() { GetIndexAlloc()->Free(slot_index_); }

  // Accessor.
  T* operator->() const noexcept { return get(); }
  T& operator*() const noexcept { return *get(); }
  T* get() const noexcept { return Get(); }

 private:
  T* Get() const noexcept {
    auto current_fiber = detail::GetCurrentFiberEntity();
      auto ptr = current_fiber->GetFiberLocalStorage(slot_index_);
      if (!*ptr) {
        *ptr = MakeErased<T>();
      }
      return static_cast<T*>(ptr->Get());
  }

  static IndexAlloc* GetIndexAlloc() {
      return IndexAlloc::Instance();
  }

 private:
  std::size_t slot_index_;
};

} // namespace tinyRPC::fiber

#endif