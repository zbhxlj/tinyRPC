#ifndef _SRC_INIT_H_
#define _SRC_INIT_H_

#include "base/Function.h"

namespace tinyRPC {

// Initialize flare runtime, call user's callback, and destroy flare's runtime.
//
// If necessary, you need to capture `envp` yourself.
//
// `argc` / `argv` passed to `cb` might be different from what's given to
// `Start`, as some library consumes them (gflags, for example). If the original
// one is needed, you need to capture them yourself.
//
// Return value of `cb` is returned as is, any failures in flare runtime lead to
// `abort()`. If `cb` returns `void`, it's treated as always returning 0.
int Start(int argc, char** argv, UniqueFunction<int(int, char**)> cb);

// Block caller until exit signal is received. Only usable inside Flare's
// runtime (i.e., (indirectly) called by `Start`.)
//
// Calling this method results in `SIGINT` / `SIGQUIT` / `SIGTERM` being
// captured by Flare's runtime.
void WaitForQuitSignal();

// Check if exit signal is received. Only usable inside Flare's runtime (i.e.,
// (indirectly) called by `Start`.)
//
// Calling this method results in `SIGINT` / `SIGQUIT` / `SIGTERM` being
// captured by Flare's runtime.
bool CheckForQuitSignal();

// In certain cases, you use utilities in `flare/base` but is not ready for
// converting the entire project to a Flare-native one, you can use these two
// method to do "minimal" initialization of Flare runtime to use them.
//
// By "minimum", only some internal background workers are started, none of
// fiber runtime / any "meaningful" utility (e.g. `flare/base/option.h` /
// `flare/base/monitoring.h`) is initialized. You need to initialize them
// yourself.
//
// YOU SHOULD AVOID DOING THIS AND RESORT TO `flare::Start(...)` WHEN POSSIBLE.
void InitializeBasicRuntime();  // Crash on failure.
void TerminateBasicRuntime();

}  // namespace tinyRPC

#endif  
