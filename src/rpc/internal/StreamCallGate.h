#ifndef _SRC_RPC_INTERNAL_STREAM_CALL_GATE_H_
#define _SRC_RPC_INTERNAL_STREAM_CALL_GATE_H_

#include <chrono>
#include <limits>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "gflags/gflags.h"
#include "gtest/gtest_prod.h"

#include "../../base/Function.h"
#include "../../base/MaybeOwning.h"
#include "../../base/Endpoint.h"
#include "../../fiber/Mutex.h"
#include "../../io/EventLoop.h"
#include "../../io/native/StreamConnection.h"
#include "CorrelationID.h"
#include "CorrelationMap.h"
#include "../protocol/Message.h"
#include "../protocol/Controller.h"
#include "../protocol/StreamProtocol.h"

namespace tinyRPC::rpc::internal {

// A "call gate" owns a connection, i.e., no load balance / fault tolerance /
// name resolving will be done here. Use `XxxChannel` instead if that's what
// you want.
//
// This is generally used by `XxxChannel` (via `StreamCallGatePool`).
//
// Thread-safe.
class StreamCallGate : public StreamConnectionHandler {
 public:
  using MessagePtr = std::unique_ptr<Message>;

  // Timestamps. Not applicable to streaming RPC.
  struct Timestamps {
    std::uint64_t sent_tsc;
    std::uint64_t received_tsc;
    std::uint64_t parsed_tsc;
  };

  // Final status of an RPC.
  enum class CompletionStatus { Success, IoError, ParseError, Timeout };

  // Arguments for making a fast call.
  struct FastCallArgs {
    // `msg_on_success` is applicable only if `status` is `Success`.
    UniqueFunction<void(CompletionStatus status, MessagePtr msg_on_success,
                  const Timestamps& timestamps)> completion;

    // Passed to protocol object, opaque to us.
    Controller* controller;
  };

  struct Options {
    MaybeOwning<StreamProtocol> protocol;
    std::size_t maximum_packet_size = 0;
  };

  StreamCallGate();
  ~StreamCallGate();

  // On failure, the call gate is set to "unhealthy" state. Healthy state can be
  // checked via `Healthy()`.
  //
  // We don't return failure from this method to simplify implementation of our
  // caller. Handling failure of gate open can be hard. Besides, failure should
  // be rare, most of the failures are a result of exhaustion of ephemeral
  // ports.
  void Open(const Endpoint& address, Options options);

  // Check if the call gate is healthy (i.e, it's still connected, not in an
  // error state.).
  bool Healthy() const;

  // Manually mark this call gate as unhealthy.
  void SetUnhealthy();

  // All pending RPCs are immediately completed with `kGateClosing`.
  void Stop();
  void Join();

  // Some basics about the call gate.
  const Endpoint& GetEndpoint() const;
  const StreamProtocol& GetProtocol() const;

  // Fast path for simple RPCs (one request / one response).
  //
  // 64-bit correlation ID is NOT supported. AFAICT we don't generate 64-bit
  // correlation ID in our system.
  void FastCall(const Message& m, std::shared_ptr<FastCallArgs> args,
                std::chrono::steady_clock::time_point timeout);

  // Cancel a previous call to `FastCall`.
  //
  // Returns `nullptr` is the call has already been completed (e.g., by
  // receiving its response from network).
  std::shared_ptr<FastCallArgs> CancelFastCall(std::uint32_t correlation_id);


 public:
  // Get event loop this gate is attached to.
  //
  // FOR INTERNAL USE ONLY.
  EventLoop* GetEventLoop();

 private:
  FRIEND_TEST(StreamCallGatePoolTest, RemoveBrokenGate);

  struct FastCallContext {
    fiber::Mutex lock;

    std::uint64_t correlation_id;
    tinyRPC::fiber::detail::TimerPtr timeout_timer;
    Timestamps timestamps;
    std::shared_ptr<FastCallArgs> user_args;
  };


  // Called in dedicated fiber. Blocking is OK.
  void ServiceFastCallCompletion(std::unique_ptr<Message> msg,
                                 std::shared_ptr<FastCallContext> ctx,
                                 std::uint64_t tsc);

  void OnAttach(StreamConnection*) override;  // Not cared.
  void OnDetach() override;                   // Not cared.
  void OnWriteBufferEmpty() override;         // Not cared.

  // Called upon data is written out.
  //
  // `ctx` is correlation id if it's associated with a stream, 0 otherwise.
  void OnDataWritten(std::uintptr_t ctx) override;

  // Called upon new data is available.
  DataConsumptionStatus OnDataArrival(std::string& buffer) override;

  // Called upon remote close.  All outstanding RPCs are completed with error.
  void OnClose() override;

  // Called upon error. All outstanding RPCs are completed with error.
  void OnError() override;

  // Initialize underlying connection.
  bool InitializeConnection(const Endpoint& ep);

  // Write data out. This method also reconnect the underlying socket if it's
  // closed.
  bool WriteOut(std::string& buffer, std::uintptr_t ctx);

  // Allocate a context associated with `correlation_id`. `f` is called to
  // initialize the context.
  //
  // Initialization of the new call context is sequenced before its insertion
  // into the map.
  template <class F>
  void AllocateRpcContextFastCall(std::uint32_t correlation_id, F&& init);

  // Reclaim rpc context if `correlation_id` is associated with a fast call,
  // otherwise `nullptr` is returned.
  //
  // Note that this method also returns `nullptr` if `correlation_id` does not
  // exist at all. This may somewhat degrade performance of processing
  // streams, we might optimize it some day later.
  std::shared_ptr<FastCallContext> TryReclaimRpcContextFastCall(
      std::uint32_t correlation_id);

  // Traverse in-use fast-call correlations.
  //
  // `f` is called with *RPC correlation ID* (not including connection
  // correlation ID) and call context.
  template <class F>
  void ForEachRpcContextFastCall(F&& f);



  // Serialize `message`.
  std::string WriteMessage(const Message& message,
                                   Controller* controller) const;

  // Raise an error if the corresponding RPC is found.
  static void RaiseErrorIfPresentFastCall(
      CorrelationMap<std::shared_ptr<FastCallContext>>* map,
      std::uint32_t conn_correlation_id, std::uint32_t rpc_correlation_id,
      CompletionStatus status);

  // Complete all on-going RPCs with failure status.
  void UnsafeRaiseErrorGlobally();


 private:
  Options options_{};
  Endpoint endpoint_;
  EventLoop* event_loop_ = nullptr;
  std::shared_ptr<NativeStreamConnection> conn_;
  std::atomic<bool> healthy_{true};

  // Connection correlation ID. Fast-calls need this to access correlation map.
  std::uint32_t conn_correlation_id_{NewConnectionCorrelationId()};
  CorrelationMap<std::shared_ptr<FastCallContext>>* correlation_map_;
};

}  // namespace tinyRPC

#endif  
