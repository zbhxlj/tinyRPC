#ifndef _SRC_RPC_INTERNAL_STREAM_CALL_GATE_POOL_H_
#define _SRC_RPC_INTERNAL_STREAM_CALL_GATE_POOL_H_

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gflags/gflags_declare.h"

#include "../../base/Endpoint.h"
#include "../../fiber/Timer.h"

DECLARE_int32(flare_rpc_client_max_connections_per_server);
DECLARE_int32(flare_rpc_client_connection_max_idle);

namespace tinyRPC::rpc::internal {

class StreamCallGate;

// RAII wrapper for `StreamCallGate*`.
class StreamCallGateHandle {
 public:
  StreamCallGateHandle();

  // Note that you should only call `release` if the gate is still usable
  // (`Healthy()` holds), otherwise you should free the gate instead of return
  // it back to the pool.
  StreamCallGateHandle(std::shared_ptr<StreamCallGate> p);
  ~StreamCallGateHandle();

  // Move-able only.
  StreamCallGateHandle(StreamCallGateHandle&&) noexcept;
  StreamCallGateHandle& operator=(StreamCallGateHandle&&) noexcept;
  StreamCallGateHandle(const StreamCallGateHandle&) = delete;
  StreamCallGateHandle& operator=(const StreamCallGateHandle&) = delete;

  // Accessor.
  StreamCallGate* Get() const { return ptr_.get(); }
  StreamCallGate* operator->() const { return Get(); }
  StreamCallGate& operator*() const { return *Get(); }
  explicit operator bool() const noexcept { return !!Get(); }

  void Close() noexcept;

 private:
  std::shared_ptr<StreamCallGate> ptr_;
};

}  // namespace tinyRPC::rpc::internal

#endif  
