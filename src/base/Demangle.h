#ifndef _SRC_BASE_DEMANGLE_H_
#define _SRC_BASE_DEMANGLE_H_

#include <string>
#include <typeinfo>
#include <utility>

namespace tinyRPC {

std::string Demangle(const char* s);

template <class T>
std::string GetTypeName() {
  return Demangle(typeid(T).name());
}

template <class T>
std::string GetTypeName(T&& o) {
  return Demangle(typeid(std::forward<T>(o)).name());
}

}  // namespace tinyRPC

#endif  
