#include "StreamCallGateHandle.h"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "gflags/gflags.h"

#include "../../base/chrono.h"
#include "../../base/Random.h"
#include "../../base/Latch.h"
#include "../../fiber/Runtime.h"
#include "../../fiber/ThisFiber.h"
#include "../../io/EventLoop.h"
#include "StreamCallGate.h"
#include "StreamCallGateHandle.h"

DEFINE_int32(flare_rpc_client_max_connections_per_server, 8,
             "Maximum connections per server. This number is round down to "
             "number of worker groups internally. This option only affects "
             "connections to server whose protocol supports multiplexing. Note "
             "that if you're using two different protocols to call a server, "
             "the connections are counted separately (i.e., there will be at "
             "most as two times connections as the limit specified here.).");
DEFINE_int32(flare_rpc_client_remove_idle_connection_interval, 15,
             "Interval, in seconds, between to run of removing client-side "
             "idle connections.");
// The client must close the connection before the server, otherwise we risk
// using a connection that has been (or, is being) closed by the server.
DEFINE_int32(
    flare_rpc_client_connection_max_idle, 45,
    "Time period before recycling a client-side idle connection, in seconds.");

using namespace std::literals;

namespace tinyRPC::rpc::internal {

StreamCallGateHandle::StreamCallGateHandle() = default;

StreamCallGateHandle::StreamCallGateHandle(std::shared_ptr<StreamCallGate> p)
    : ptr_(std::move(p)) {}

StreamCallGateHandle::~StreamCallGateHandle() { Close(); }

StreamCallGateHandle::StreamCallGateHandle(StreamCallGateHandle&&) noexcept =
    default;
StreamCallGateHandle& StreamCallGateHandle::operator=(
    StreamCallGateHandle&&) noexcept = default;

void StreamCallGateHandle::Close() noexcept {}

}  // namespace tinyRPC::rpc::internal
