#include "Demangle.h"

#ifdef __GNUC__
#include <cxxabi.h>
#endif

#include <string>

#include "ScopedDeferred.h"

namespace tinyRPC {

std::string Demangle(const char* s) {
#ifdef _MSC_VER
  return s;
#elif defined(__GNUC__)
  [[maybe_unused]] int status;
  auto demangled = abi::__cxa_demangle(s, nullptr, nullptr, &status);
  ScopedDeferred _([&] { free(demangled); });
  if (!demangled) {
    return s;
  }
  return demangled;
#else
  #error "Demangle not supported on current compiler."
#endif
}

}  // namespace tinyRPC
