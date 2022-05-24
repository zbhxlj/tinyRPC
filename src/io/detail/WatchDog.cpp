#include "WatchDog.h"

#include <chrono>
#include <memory>
#include <vector>

#include "../../include/gflags/gflags.h"

#include "../../base/chrono.h"
#include "../../base/Latch.h"
#include "../EventLoop.h"

using namespace std::literals;

DEFINE_int32(
    watchdog_check_interval, 10000,
    "Interval between two run of the watch dog, in milliseconds. This value "
    "should be at least as large as `flare_watchdog_maximum_tolerable_delay`.");

DEFINE_int32(watchdog_maximum_tolerable_delay, 5000,
             "Maximum delay between watch dog posted its callback and its "
             "callback being called, in milliseconds.");

namespace tinyRPC::io::detail {

Watchdog::Watchdog() = default;

void Watchdog::AddEventLoop(EventLoop* watched) { watched_.push_back(watched); }

void Watchdog::Start() {
  watcher_ = std::thread([&] { return WorkerProc(); });
}

void Watchdog::Stop() {
  exiting_.store(true);
  latch_.count_down();
}

void Watchdog::Join() { watcher_.join(); }

void Watchdog::WorkerProc() {
  FLARE_CHECK_GE(FLAGS_watchdog_check_interval,
                 FLAGS_watchdog_maximum_tolerable_delay);
  auto next_try = ReadSteadyClock();

  // For every `FLAGS_watchdog_check_interval` second, we post a task to
  // `EventLoop`, and check if it get run within
  // `FLAGS_watchdog_maximum_tolerable_delay` seconds.
  while (!exiting_) {
    auto wait_until =
        ReadSteadyClock() + FLAGS_watchdog_maximum_tolerable_delay * 1ms;
    // Be careful here, when `flare_watchdog_crash_on_unresponsive` is disabled,
    // `acked` can be `count_down`-d **after** we leave the scope.
    //
    // We use `std::shared_ptr` here to address this.
    std::vector<std::shared_ptr<Latch>> acked(watched_.size());

    for (std::size_t index = 0; index != watched_.size(); ++index) {
      acked[index] = std::make_shared<Latch>(1);

      // Add a task to the `EventLoop` and check (see below) if it get run in
      // time.
      watched_[index]->AddTask([acked = acked[index]] { acked->count_down(); });
    }

    // This loop may not be merged with the above one, as it may block (and
    // therefore delays subsequent calls to `EventLoop::AddTask`.).
    for (std::size_t index = 0; index != watched_.size(); ++index) {
      bool responsive = acked[index]->wait_until(wait_until) ||
                        exiting_;
      if (!responsive) {
          FLARE_LOG_FATAL(
              "Event loop {} is likely unresponsive. Crashing the program.",
              fmt::ptr(watched_[index]));
        }
      }
    }
    VLOG(10) << "Watchdog: Life is good.";

    // Sleep until next round starts.
    next_try += FLAGS_watchdog_check_interval * 1ms;
    latch_.wait_until(next_try);
  }
}  // namespace tinyRPC::io::detail
