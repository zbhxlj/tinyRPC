#include "StreamCallGate.h"

#include <chrono>
#include <limits>
#include <utility>
#include <vector>
#include <memory>

#include "gflags/gflags.h"

// #include "flare/base/internal/early_init.h"
#include "../../base/Logging.h"
#include "../../fiber/Fiber.h"
#include "../../fiber/Runtime.h"
#include "../../fiber/ThisFiber.h"
#include "../../fiber/Timer.h"
#include "../../io/EventLoop.h"
#include "../../io/native/StreamConnection.h"
#include "../../io/util/Socket.h"
#include "../../base/Tsc.h"

namespace tinyRPC {

namespace rpc::internal {

StreamCallGate::StreamCallGate() = default;

StreamCallGate::~StreamCallGate() = default;

void StreamCallGate::Open(const Endpoint& address, Options options) {
  options_ = std::move(options);
  endpoint_ = address;
  correlation_map_ = GetCorrelationMapFor<std::shared_ptr<FastCallContext>>(
      fiber::GetCurrentSchedulingGroupIndex());

  FLARE_CHECK(options_.protocol);

  if (!InitializeConnection(address)) {
    SetUnhealthy();
  }
}

bool StreamCallGate::Healthy() const {
  return healthy_;
}

void StreamCallGate::SetUnhealthy() {
  healthy_ = false;
}

void StreamCallGate::Stop() {
  UnsafeRaiseErrorGlobally();
  if (conn_) {
    conn_->Stop();
  }
}

void StreamCallGate::Join() {
  if (conn_) {
    conn_->Join();
  }
}

const Endpoint& StreamCallGate::GetEndpoint() const { return endpoint_; }

const StreamProtocol& StreamCallGate::GetProtocol() const {
  return *options_.protocol;
}

void StreamCallGate::FastCall(const Message& m, std::shared_ptr<FastCallArgs> args,
                              std::chrono::steady_clock::time_point timeout) {
  FLARE_CHECK_LE(m.GetCorrelationId(),
                 std::numeric_limits<std::uint32_t>::max(),
                 "Unsupported: 64-bit RPC correlation ID.");

  // Serialization is done prior to fill `Timestamps.sent_tsc`.
  auto serialized = WriteMessage(m, args->controller);

  {
    // This lock guarantees us that no one else races with us.
    //
    // The race is unlikely but possible:
    //
    // 1. We allocated the context for this packet.
    // 2. Before we finished initializing this context (and before we send this
    //    packet out), the remote side (possibly erroneously) sends us a packet
    //    with the same correlation ID this packet carries, and triggers
    //    incoming packet callback.
    // 3. The callback removes the timeout timer.
    //
    // In this case, we'd risk use-after-free when enabling the timeout timer
    // later.
    std::unique_lock<fiber::Mutex> ctx_lock;
    fiber::detail::TimerPtr timeout_timer = 0;

    if (timeout != std::chrono::steady_clock::time_point::max()) {
      auto timeout_cb = [map = correlation_map_,
                         conn_cid = conn_correlation_id_,
                         rpc_cid = m.GetCorrelationId()](fiber::detail::TimerPtr& timerPtr) mutable {
        RaiseErrorIfPresentFastCall(map, conn_cid, rpc_cid,
                                    CompletionStatus::Timeout);
      };
      timeout_timer =
          fiber::internal::CreateTimer(timeout, std::move(timeout_cb));
    }

    // Initialize call context in call map.
    AllocateRpcContextFastCall(m.GetCorrelationId(), [&](FastCallContext* ctx) {
      ctx_lock = std::unique_lock(ctx->lock);

      ctx->timestamps.sent_tsc = ReadTsc();  // Not exactly.
      ctx->timeout_timer = timeout_timer;
      ctx->user_args = std::move(args);
    });
    if (timeout_timer) {
      fiber::internal::EnableTimer(timeout_timer);
    }
  }

  // Raise an error early if the connection is not healthy.
  if (!healthy_) {
    RaiseErrorIfPresentFastCall(correlation_map_, conn_correlation_id_,
                                m.GetCorrelationId(),
                                CompletionStatus::IoError);
  } else {
    WriteOut(serialized, 0);
  }
}

std::shared_ptr<StreamCallGate::FastCallArgs> StreamCallGate::CancelFastCall(
    std::uint32_t correlation_id) {
  auto ptr = TryReclaimRpcContextFastCall(correlation_id);
  return ptr ? std::move(ptr->user_args) : nullptr;
}

bool StreamCallGate::InitializeConnection(const Endpoint& ep) {
  // Initialize socket.
  auto fd = io::util::CreateStreamSocket(ep.Family());
  if (!fd) {
    FLARE_LOG_ERROR("Failed to create socket with AF {}.",
                                 ep.Family());
    return false;
  }
  io::util::SetCloseOnExec(fd.Get());
  io::util::SetNonBlocking(fd.Get());
  io::util::SetTcpNoDelay(fd.Get());
  // `io::util::SetSendBufferSize` & `io::util::SetReceiveBufferSize`?
  if (!io::util::StartConnect(fd.Get(), ep)) {
    FLARE_LOG_WARNING("Failed to connection to [{}].",
                                   ep.ToString());
    return false;
  }

  // Initialize connection.
  NativeStreamConnection::Options opts;
  opts.read_buffer_size = options_.maximum_packet_size;
  conn_ =
      std::make_shared<NativeStreamConnection>(std::move(fd), std::move(opts));

  // Add the connection to event loop.
  event_loop_ =
      GetGlobalEventLoop(fiber::GetCurrentSchedulingGroupIndex());
  event_loop_->AttachDescriptor(conn_, false);

  // `conn_`'s callbacks may access `conn_` itself, so we must delay enabling
  // the descriptor until `conn_` is also initialized.
  event_loop_->EnableDescriptor(conn_.get());

  conn_->StartHandshaking();

  return true;
}

bool StreamCallGate::WriteOut(std::string& buffer, std::uintptr_t ctx) {
  return conn_->Write(std::move(buffer), ctx);
}

// Allocate a context associated with `correlation_id`. `f` is called to
// initialize the context.
template <class F>
void StreamCallGate::AllocateRpcContextFastCall(std::uint32_t correlation_id,
                                                F&& init) {
  auto ptr = std::make_shared<FastCallContext>();
  std::forward<F>(init)(&*ptr);

  correlation_map_->Insert(
      MergeCorrelationId(conn_correlation_id_, correlation_id), std::move(ptr));
}

// Reclaim rpc context if `correlation_id` is associated with a fast call,
// otherwise `nullptr` is returned.
//
// Note that this method also returns `nullptr` if `correlation_id` does not
// exist at all. This may somewhat degrade performance of processing
// streams, we might optimize it some day later.
std::shared_ptr<StreamCallGate::FastCallContext>
StreamCallGate::TryReclaimRpcContextFastCall(std::uint32_t correlation_id) {
  return correlation_map_->Remove(
      MergeCorrelationId(conn_correlation_id_, correlation_id));
}

// Traverse in-use fast-call correlations.
template <class F>
void StreamCallGate::ForEachRpcContextFastCall(F&& f) {
  correlation_map_->ForEach([&](auto key, auto&& v) {
    auto&& [conn, rpc] = SplitCorrelationId(key);
    if (conn == conn_correlation_id_) {
      f(rpc, v);
    }
  });
}



EventLoop* StreamCallGate::GetEventLoop() { return event_loop_; }

// Called in dedicated fiber. Blocking is OK.
void StreamCallGate::ServiceFastCallCompletion(std::unique_ptr<Message> msg,
                                               std::shared_ptr<FastCallContext> ctx,
                                               std::uint64_t tsc) {
  ctx->timestamps.received_tsc = tsc;
  {
    // Wait until the context is fully initialized (if not yet).
    std::scoped_lock _(ctx->lock);
  }

  if (auto t = std::exchange(ctx->timeout_timer, nullptr)) {  // We set a timer.
    fiber::internal::KillTimer(t);
  }

  auto user_args = std::move(ctx->user_args);
  auto cb = [&] {
    auto ctlr = user_args->controller;
    auto f = std::move(user_args->completion);
    std::unique_ptr<Message> parsed =
        options_.protocol->TryParse(&msg, ctlr) ? std::move(msg) : nullptr;

    ctx->timestamps.parsed_tsc = ReadTsc();
    if (parsed) {
      f(CompletionStatus::Success, std::move(parsed), ctx->timestamps);
    } else {
      f(CompletionStatus::ParseError, nullptr, ctx->timestamps);
    }
  };

    cb();

  // `*this` may not be touched since now as user's callback might have already
  // freed us.
}


StreamConnectionHandler::DataConsumptionStatus StreamCallGate::OnDataArrival(
    std::string& buffer) {
  auto arrival_tsc = ReadTsc();
  bool ever_suppressed = false;
  while (!buffer.empty()) {
    std::unique_ptr<Message> m;
    auto rc = options_.protocol->TryCutMessage(buffer, &m);

    if (rc == StreamProtocol::MessageCutStatus::ProtocolMismatch ||
        rc == StreamProtocol::MessageCutStatus::Error) {
      FLARE_LOG_WARNING(
          "Failed to cut message off from connection to [{}]. Closing.",
          endpoint_.ToString());
      return DataConsumptionStatus::Error;
    } else if (rc == StreamProtocol::MessageCutStatus::NotIdentified ||
               rc == StreamProtocol::MessageCutStatus::NeedMore) {
      return !ever_suppressed ? DataConsumptionStatus::Ready
                              : DataConsumptionStatus::SuppressRead;
    }
    FLARE_CHECK(rc == StreamProtocol::MessageCutStatus::Cut);

    // Dispatch the message.
    auto correlation_id = m->GetCorrelationId();

    // TODO(luobogao): We could infer the message type (fast call or stream) by
    // examining `message->GetType()` and move the rest into dedicated fiber.
    // However, that way we'd have a hard time in removing the timeout timer.
    // We might have to refactor timer's interface to resolve this.
    if (auto ctx = TryReclaimRpcContextFastCall(correlation_id)) {
      // It's a fast call then.
      if (FLARE_UNLIKELY(m->GetType() != Message::Type::Single)) {
        FLARE_LOG_WARNING(
            "Message #{} is marked as part of a stream, but we're expecting a "
            "normal RPC response.",
            m->GetCorrelationId());
        continue;
      }
      // FIXME: We need to wait for the callback to return before we could be
      // destroyed.
      fiber::StartFiberDetached([this, tsc = arrival_tsc,
                                           msg = std::move(m),
                                           ctx = std::move(ctx)]() mutable {
        ServiceFastCallCompletion(std::move(msg), std::move(ctx), tsc);
      });
    } 
  }  // Loop until no more message could be cut off.
  return !ever_suppressed ? DataConsumptionStatus::Ready
                          : DataConsumptionStatus::SuppressRead;
}

void StreamCallGate::OnAttach(StreamConnection*) {}
void StreamCallGate::OnDetach() {}
void StreamCallGate::OnWriteBufferEmpty() {}
void StreamCallGate::OnClose() { OnError(); }
void StreamCallGate::OnError() {
  healthy_.store(false, std::memory_order_release);
  UnsafeRaiseErrorGlobally();
}

std::string StreamCallGate::WriteMessage(const Message& message,
                                                 Controller* controller) const {
  std::string serialized;

  options_.protocol->WriteMessage(message, serialized, controller);
  return serialized;
}

// CAUTION: THIS METHOD CAN BE CALLED EITHER FROM PTHREAD CONTEXT (ON TIMEOUT)
// OR FIBER CONTEXT (ON IO ERROR.).
void StreamCallGate::RaiseErrorIfPresentFastCall(
    CorrelationMap<std::shared_ptr<FastCallContext>>* map,
    std::uint32_t conn_correlation_id, std::uint32_t rpc_correlation_id,
    CompletionStatus status) {
  auto ctx =
      map->Remove(MergeCorrelationId(conn_correlation_id, rpc_correlation_id));

  if (!ctx) {  // Completed in the mean time?
    return;    // Nothing then.
  }

  fiber::StartFiberDetached([ctx = std::move(ctx), status]() mutable {
    {
      // Make sure the context is fully initialized.
      std::scoped_lock _(ctx->lock);
    }
    if (auto t = std::exchange(ctx->timeout_timer, nullptr)) {
      fiber::internal::KillTimer(t);
    }
    auto user_args = std::move(ctx->user_args);
    FLARE_CHECK(!!user_args->completion);
    auto f = std::move(user_args->completion);
    {
      static Timestamps timeStamp;
      f(status, nullptr, timeStamp);
    }
  });
}


void StreamCallGate::UnsafeRaiseErrorGlobally() {
  std::vector<std::uint64_t> fast_cids;
  ForEachRpcContextFastCall(
      [&](auto&& cid, auto&& ctx) { fast_cids.push_back(cid); });

  for (auto&& c : fast_cids) {
    RaiseErrorIfPresentFastCall(correlation_map_, conn_correlation_id_, c,
                                CompletionStatus::IoError);
  }
}

}  // namespace rpc::internal

}  // namespace tinyRPC
