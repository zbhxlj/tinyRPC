#ifndef _SRC_INIT_ON_INIT_H_
#define _SRC_INIT_ON_INIT_H_

#include <cstdint>
#include <utility>

#include "../base/Function.h"
#include "../base/internal/Macro.h"

// Usage:
//
// - FLARE_ON_INIT(init, fini = nullptr): This overload defaults `priority` to
//   0.
// - FLARE_ON_INIT(priority, init, fini = nullptr)
//
// This macro registers a callback that is called in `flare::Start` (after
// `main` is entered`.). The user may also provide a finalizer, which is called
// before leaving `flare::Start`, in opposite order.
//
// `priority` specifies relative order between callbacks. Callbacks with smaller
// `priority` are called earlier. Order between callbacks with same priority is
// unspecified and may not be relied on.
//
// It explicitly allowed to use this macro *without* carring a dependency to
// `//flare:init`.
//
// For UT writers: If, for any reason, you cannot use `FLARE_TEST_MAIN` as your
// `main` in UT, you need to call `RunAllInitializers()` / `RunAllFinalizers()`
// yourself when entering / leaving `main`.
#define FLARE_ON_INIT(...)                                                 \
  static ::tinyRPC::detail::OnInitRegistration FLARE_INTERNAL_PP_CAT(        \
      flare_on_init_registration_object_, __COUNTER__)(__FILE__, __LINE__, \
                                                       __VA_ARGS__);

namespace tinyRPC::internal {

// Registers a callback that's called before leaving `flare::Start`.
//
// These callbacks are called after all finalizers registered via
// `FLARE_ON_INIT`.
void SetAtExitCallback(UniqueFunction<void()> callback);

}  // namespace flare::internal

// Implementation goes below.
namespace tinyRPC::detail {

// Called by `OnInitRegistration`.
void RegisterOnInitCallback(std::int32_t priority, UniqueFunction<void()> init,
                            UniqueFunction<void()> fini);

// Called by `flare::Start`.
void RunAllInitializers();
void RunAllFinalizers();

// Helper class for registering initialization callbacks at start-up time.
class OnInitRegistration {
 public:
  OnInitRegistration(const char* file, std::uint32_t line,
                     UniqueFunction<void()> init, UniqueFunction<void()> fini = nullptr)
      : OnInitRegistration(file, line, 0, std::move(init), std::move(fini)) {}

  OnInitRegistration(const char* file, std::uint32_t line,
                     std::int32_t priority, UniqueFunction<void()> init,
                     UniqueFunction<void()> fini = nullptr) {
    RegisterOnInitCallback(priority, std::move(init), std::move(fini));
  }
};

}  // namespace tinyRPC::detail

#endif  
