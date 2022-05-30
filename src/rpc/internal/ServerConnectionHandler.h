#ifndef _SRC_RPC_INTERNAL_SERVER_CONNECTION_HANDLER_H_
#define _SRC_RPC_INTERNAL_SERVER_CONNECTION_HANDLER_H_

#include <atomic>
#include <chrono>

#include "../../base/chrono.h"
#include "../../base/Likely.h"
#include "../../io/StreamConnection.h"

namespace tinyRPC::rpc::detail {

// Base of connection handlers used by `Server`.
class ServerConnectionHandler : public StreamConnectionHandler {
  static constexpr auto kLastEventTimestampUpdateInterval =
      std::chrono::seconds(1);

 public:
  virtual void Stop() = 0;
  virtual void Join() = 0;

 // TODO:
  std::chrono::steady_clock::time_point GetCoarseLastEventTimestamp()
      const noexcept {
    return std::chrono::steady_clock::time_point{
        next_update_.load() - kLastEventTimestampUpdateInterval};
  }

 protected:
  void ConsiderUpdateCoarseLastEventTimestamp() const noexcept {
    auto now = ReadSteadyClock().time_since_epoch();

    // We only write to `next_update_` after sufficient long period has passed
    // so as not to cause too many cache coherency traffic.
    if (FLARE_UNLIKELY(next_update_.load() < now)) {
      next_update_.store(now + kLastEventTimestampUpdateInterval);
    }
  }

 private:
  mutable std::atomic<std::chrono::nanoseconds> next_update_{
      ReadSteadyClock().time_since_epoch() +
      kLastEventTimestampUpdateInterval};
};

}  // namespace tinyRPC::rpc::detail

#endif  
