#ifndef _SRC_BASE_INTERNAL_LAZY_INIT_H_
#define _SRC_BASE_INTERNAL_LAZY_INIT_H_

#include "NeverDestroyed.h"

namespace tinyRPC::internal {

// In most case you WON'T need this, function-local static should work well.
// This is the reason why this method is put in `internal::`.
//
// However, for our implementation's sake, `LazyInit<T>()` could be handy in
// defining objects that could (or should) be shared globally. (For
// deterministic initialization, simplification of code, or whatever else
// reasons.)
//
// Note that instance returned by this method is NEVER destroyed.
template <class T, class Tag = void>
T* LazyInit() {
  static NeverDestroyed<T> t;
  return t.Get();
}

// The instance returned by this method is NOT the same instance as of returned
// by `LazyInit`.
//
// This method can be especially useful when you want to workaround [this issue
// (using in-class initialization of NSDM and default argument at the same
// time)](https://stackoverflow.com/a/17436088).
//
// Note that instance returned by this method is NEVER destroyed.
template <class T, class Tag = void>
const T& LazyInitConstant() {
  static NeverDestroyed<T> t;
  return *t;
}

}  // namespace tinyRPC::internal

#endif  
