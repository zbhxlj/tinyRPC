#include "init.h"

#include <sys/signal.h>
#include <mutex>
#include <chrono>

#include "../../include/gflags/gflags.h"
#include "../../include/glog/logging.h"
#include "../../include/glog/raw_logging.h"

// #include "flare/base/internal/time_keeper.h"
#include "base/Logging.h"
#include "base/Random.h"
#include "base/Latch.h"
#include "fiber/Fiber.h"
#include "fiber/Runtime.h"
#include "fiber/ThisFiber.h"
// #include "flare/init/on_init.h"
#include "io/EventLoop.h"

using namespace std::literals;

namespace tinyRPC {

namespace {

std::atomic<bool> g_quit_signal {false};

void QuitSignalHandler(int sig) {
  if(g_quit_signal.exchange(true)){
    RAW_LOG(FATAL, "Double quit signal received. Crashing the program.");
  }
}

void InstallQuitSignalHandler() {
  static std::once_flag f;

  std::call_once(f, [&] {
    FLARE_PCHECK(signal(SIGINT, QuitSignalHandler) != SIG_ERR);   // [Ctrl + C]
    FLARE_PCHECK(signal(SIGQUIT, QuitSignalHandler) != SIG_ERR);  // [Ctrl + \]
    FLARE_PCHECK(signal(SIGTERM, QuitSignalHandler) != SIG_ERR);
  });
}

}  // namespace 

// Crash on failure.
int Start(int argc, char** argv, UniqueFunction<int(int, char**)> cb) {
  google::InstallFailureSignalHandler();

  google::ParseCommandLineFlags(&argc, &argv, true);
  
  google::InitGoogleLogging(argv[0]);

  // No you can't install a customized `Future` executor to run `Future`'s
  // continuations in new fibers.
  //
  // The default executor change has a global (not only in flare's context)
  // effect, and will likely break program if `Future`s are also used in pthread
  // context.
  //
  // DO NO INSTALL A CUSTOMIZED EXECUTOR TO RUN `FUTURE` IN NEW FIBER.

  // This is a bit late, but we cannot write log (into file) before glog is
  // initialized.
  FLARE_LOG_INFO("Flare started.");

  FLARE_PCHECK(signal(SIGPIPE, SIG_IGN) != SIG_ERR);

  InitializeBasicRuntime();
  // detail::RunAllInitializers();
  fiber::StartRuntime();

  FLARE_LOG_INFO("Flare runtime initialized.");

  // Now we start to run in fiber environment.
  int rc = 0;
  {
    Latch l(1);
    fiber::StartFiberDetached([&] {
      StartAllEventLoops();

      rc = cb(argc, argv);  // User's callback.

      StopAllEventLoops();
      JoinAllEventLoops();

      l.count_down();
    });  // Don't `join()` here, we can't use fiber synchronization
         // primitives outside of fiber context.
    l.wait();
  }

  fiber::TerminateRuntime();
  // detail::RunAllFinalizers();
  TerminateBasicRuntime();

  FLARE_LOG_INFO("Exited");
  return rc;
}

void WaitForQuitSignal() {
  // We only capture quit signal(s) if we're called. This allows users to handle
  // these signals themselves (by not calling this method) if they want.
  InstallQuitSignalHandler();

  while (!g_quit_signal) {
    this_fiber::SleepFor(100ms);
  }
  FLARE_LOG_INFO("Quit signal received.");
}

bool CheckForQuitSignal() {
  InstallQuitSignalHandler();
  return g_quit_signal;
}

void InitializeBasicRuntime() {
  // internal::BackgroundTaskHost::Instance()->Start();
  // internal::TimeKeeper::Instance()->Start();
}

void TerminateBasicRuntime() {
  // internal::TimeKeeper::Instance()->Stop();
  // internal::TimeKeeper::Instance()->Join();
  // internal::BackgroundTaskHost::Instance()->Stop();
  // internal::BackgroundTaskHost::Instance()->Join();
}

}  // namespace tinyRPC
