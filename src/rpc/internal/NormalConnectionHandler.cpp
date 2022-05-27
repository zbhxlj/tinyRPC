#include "NormalConnectionHandler.h"

#include <memory>
#include <string>
#include <utility>

#include "../../base/ScopedDeferred.h"
#include "../../base/Tsc.h"
#include "../../fiber/Latch.h"
#include "../../fiber/Fiber.h"
#include "../../fiber/ThisFiber.h"
#include "../Server.h"

using namespace std::literals;

namespace tinyRPC::rpc::detail {

NormalConnectionHandler::NormalConnectionHandler(Server* owner,
                                                 std::unique_ptr<Context> ctx)
    : owner_(owner), ctx_(std::move(ctx)) {
  FLARE_CHECK(!ctx_->services.empty(),
              "No service is enabled, confused about what to serve.");

  last_service_ = ctx_->services.front();
}

void NormalConnectionHandler::Stop() {}

void NormalConnectionHandler::Join() {
  while (ongoing_requests_) {
    this_fiber::SleepFor(100ms);
  }
}

void NormalConnectionHandler::OnAttach(StreamConnection* conn) { conn_ = conn; }

void NormalConnectionHandler::OnDetach() {}

void NormalConnectionHandler::OnWriteBufferEmpty() {}

StreamConnectionHandler::DataConsumptionStatus
NormalConnectionHandler::OnDataArrival(std::string& buffer) {
  FLARE_CHECK(conn_);  // This cannot fail.

  ScopedDeferred _([&] { ConsiderUpdateCoarseLastEventTimestamp(); });
  bool ever_suppressed = false;
  auto receive_tsc = ReadTsc();

  while (!buffer.empty()) {
    auto rc = ProcessOnePacket(buffer, receive_tsc);
    if (FLARE_LIKELY(rc == ProcessingStatus::Success)) {
      continue;
    } else if (rc == ProcessingStatus::Error) {
      return DataConsumptionStatus::Error;
    } else if (rc == ProcessingStatus::SuppressRead) {
      ever_suppressed = true;
    } else {
      FLARE_CHECK(rc == ProcessingStatus::Saturated);
      return ever_suppressed ? DataConsumptionStatus::SuppressRead
                             : DataConsumptionStatus::Ready;
    }
  }
  return ever_suppressed ? DataConsumptionStatus::SuppressRead
                         : DataConsumptionStatus::Ready;
}

void NormalConnectionHandler::OnClose() {
  FLARE_VLOG(10, "Connection from [{}] closed.", ctx_->remote_peer.ToString());

  owner_->OnConnectionClosed(ctx_->id);
}

void NormalConnectionHandler::OnError() {
  FLARE_VLOG(10, "Error on connection from [{}].",
             ctx_->remote_peer.ToString());
  OnClose();
}

void NormalConnectionHandler::OnDataWritten(std::uintptr_t ctx) {}

NormalConnectionHandler::ProcessingStatus
NormalConnectionHandler::ProcessOnePacket(std::string& buffer,
                                          std::uint64_t receive_tsc) {
  auto buffer_size_was = buffer.size();
  std::unique_ptr<Message> msg;
  StreamProtocol* protocol;
                              
  // Cut one message off without parsing it, so as to minimize blocking I/O
  // fiber.
  auto rc = TryCutMessage(buffer, &msg, &protocol);
  if (rc == ProcessingStatus::Error) {
    FLARE_LOG_ERROR_EVERY_SECOND("Unrecognized packet from [{}]. ",
                                 ctx_->remote_peer.ToString());
    return ProcessingStatus::Error;
  } else if (rc == ProcessingStatus::Saturated) {
    return ProcessingStatus::Saturated;
  }
  FLARE_CHECK(rc == ProcessingStatus::Success);
  auto pkt_size = buffer_size_was - buffer.size();  // Size of this packet.

  if (auto type = msg->GetType(); FLARE_LIKELY(type == Message::Type::Single)) {
    // Call service to handle it in separate fiber.
    //
    // FIXME: We always create start a fiber in the background to call service,
    // this is needed for better responsiveness in I/O fiber (had we used
    // `Dispatch` here, I/O fiber will be keep migrating between workers).
    // However, in this case, responsiveness of RPCs suffers. It might be better
    // if we call last service in foreground.

    // This check must be done here.
    //
    // Should we increment the in-fly RPC counter (via `OnNewCall`) in separate
    // fiber, there's no appropriate time point for us to wait for the counter
    // to monotonically decrease (since it can increase at any time, depending
    // on when that separate fiber is scheduled.).
    //
    // By increment the counter in I/O fiber, we can simply execute a
    // `Barrier()` on the event loop after we `Stop()`-ped the connection.
    if (FLARE_UNLIKELY(!OnNewCall())) {
      // This can't be done in dedicated fiber (for now). Since we didn't
      // increase ongoing request counter, were we run this method in dedicated
      // fiber, it's possible that `*this` is destroyed before the fiber is
      // finished.
      auto ctlr = NewController(*msg, protocol);
      ServiceOverloaded(std::move(msg), protocol, ctlr.get());
    } else {
      // FIXME: Too many captures hurts performance.
      fiber::StartFiberDetached([this, msg = std::move(msg), protocol,
                                           receive_tsc, pkt_size]() mutable {
        auto ctlr = NewController(*msg, protocol);
        ServiceFastCall(std::move(msg), protocol, std::move(ctlr), receive_tsc,
                        pkt_size);
        OnCallCompletion();
      });
    }
    return ProcessingStatus::Success;
  }
}

StreamProtocol::MessageCutStatus
NormalConnectionHandler::TryCutMessageUsingLastProtocol(
    std::string& buffer, std::unique_ptr<Message>* msg,
    StreamProtocol** used_protocol) {
  FLARE_CHECK(ever_succeeded_cut_msg_);
  FLARE_CHECK_LT(last_protocol_, ctx_->protocols.size());
  auto&& last_prot = ctx_->protocols[last_protocol_];
  auto rc = last_prot->TryCutMessage(buffer, msg);
  if (FLARE_LIKELY(rc == StreamProtocol::MessageCutStatus::Cut)) {
    *used_protocol = last_prot.get();
    return StreamProtocol::MessageCutStatus::Cut;
  } else if (rc == StreamProtocol::MessageCutStatus::NotIdentified ||
             rc == StreamProtocol::MessageCutStatus::NeedMore) {
    return StreamProtocol::MessageCutStatus::NeedMore;
  } else if (rc == StreamProtocol::MessageCutStatus::ProtocolMismatch) {
    return StreamProtocol::MessageCutStatus::ProtocolMismatch;
  } else if (rc == StreamProtocol::MessageCutStatus::Error) {
    return StreamProtocol::MessageCutStatus::Error;
  }
  FLARE_CHECK(0, "Unexpected status {}.", underlying_value(rc));
}

NormalConnectionHandler::ProcessingStatus
NormalConnectionHandler::TryCutMessage(std::string& buffer,
                                       std::unique_ptr<Message>* msg,
                                       StreamProtocol** used_protocol) {
  // If we succeeded in cutting off a message, we try the protocol we last used
  // first. Normally the protocol shouldn't change.
  if (FLARE_LIKELY(ever_succeeded_cut_msg_)) {
    auto rc = TryCutMessageUsingLastProtocol(buffer, msg, used_protocol);
    if (rc == StreamProtocol::MessageCutStatus::Cut) {
      return ProcessingStatus::Success;
    } else if (rc == StreamProtocol::MessageCutStatus::Error) {
      return ProcessingStatus::Error;
    } else if (rc == StreamProtocol::MessageCutStatus::NeedMore) {
      return ProcessingStatus::Saturated;
    } else {
      FLARE_CHECK(rc == StreamProtocol::MessageCutStatus::ProtocolMismatch,
                  "Unexpected status: {}.", underlying_value(rc));
      // Fallback to detect the protocol then.
    }
  }

  bool ever_need_more = false;

  // By reaching here, we have no idea which protocol is `buffer` using, so we
  // try all of the protocols.
  for (std::size_t index = 0; index != ctx_->protocols.size(); ++index) {
    auto&& protocol = ctx_->protocols[index];
    auto rc = protocol->TryCutMessage(buffer, msg);
    if (rc == StreamProtocol::MessageCutStatus::Cut) {
      *used_protocol = protocol.get();
      ever_succeeded_cut_msg_ = true;
      last_protocol_ = index;
      return ProcessingStatus::Success;
    } else if (rc == StreamProtocol::MessageCutStatus::NeedMore) {
      return ProcessingStatus::Saturated;
    } else if (rc == StreamProtocol::MessageCutStatus::Error) {
      return ProcessingStatus::Error;
    } else if (rc == StreamProtocol::MessageCutStatus::NotIdentified) {
      ever_need_more = true;
      continue;
    } else {
      FLARE_CHECK(rc == StreamProtocol::MessageCutStatus::ProtocolMismatch,
                  "Unexpected status: {}.", underlying_value(rc));
      // Loop then.
    }
  }

  // If at least one protocol need more bytes to investigate further, we tell
  // the caller this fact, otherwise an error is returned.
  return ever_need_more ? ProcessingStatus::Saturated : ProcessingStatus::Error;
}

std::unique_ptr<Controller> NormalConnectionHandler::NewController(
    const Message& message, StreamProtocol* protocol) const {
  return protocol->GetControllerFactory()->Create(message.GetType() !=
                                                  Message::Type::Single);
}

void NormalConnectionHandler::WriteOverloaded(const Message& corresponding_req,
                                              StreamProtocol* protocol,
                                              Controller* controller) {
  auto stream = corresponding_req.GetType() != Message::Type::Single;
  auto factory = protocol->GetMessageFactory();
  auto msg = factory->Create(MessageFactory::Type::Overloaded,
                             corresponding_req.GetCorrelationId(), stream);
  if (msg) {  // Note that `MessageFactory::Create` may return `nullptr`.
    WriteMessage(*msg, protocol, controller, kFastCallReservedContextId);
  }
}

std::size_t NormalConnectionHandler::WriteMessage(const Message& msg,
                                                  StreamProtocol* protocol,
                                                  Controller* controller,
                                                  std::uintptr_t ctx) const {
  ScopedDeferred _([&] { ConsiderUpdateCoarseLastEventTimestamp(); });
  std::string nb;
  protocol->WriteMessage(msg, nb, controller);
  auto bytes = nb.size();
  (void)conn_->Write(std::move(nb), ctx);  // Failure is ignored.
  return bytes;
}

void NormalConnectionHandler::ServiceFastCall(
    std::unique_ptr<Message>&& msg, StreamProtocol* protocol,
    std::unique_ptr<Controller> controller, std::uint64_t receive_tsc,
    std::size_t pkt_size) {
  auto dispatched_tsc = ReadTsc();

  // If the request has been in queue for too long, reject it now.
  if (FLARE_UNLIKELY(ctx_->max_request_queueing_delay != 0ns &&
                     DurationFromTsc(receive_tsc, dispatched_tsc) >
                         ctx_->max_request_queueing_delay)) {
    FLARE_LOG_WARNING(
        "Request #{} has been in queue for too long, rejected.",
        msg->GetCorrelationId());
    WriteOverloaded(*msg, protocol, &*controller);
    return;
  }

  // Parse the packet first.
  auto cid = msg->GetCorrelationId();
  if (FLARE_UNLIKELY(!protocol->TryParse(&msg, controller.get()))) {
    FLARE_LOG_WARNING("Failed to parse message #{}.", cid);
    return;
  }
  auto parsed_tsc = ReadTsc();
  // It was, and it should still be.
  FLARE_CHECK(msg->GetType() == Message::Type::Single);

  // Call context we'll pass to the handler.
  StreamService::Context call_context;
  call_context.incoming_packet_size = pkt_size;
  call_context.local_peer = ctx_->local_peer;
  call_context.remote_peer = ctx_->remote_peer;
  call_context.received_tsc = receive_tsc;
  call_context.dispatched_tsc = dispatched_tsc;
  call_context.parsed_tsc = parsed_tsc;
  call_context.controller = controller.get();

  // Let's see who would be able to handle this message.
  StreamService::InspectionResult inspection_result;
  auto handler =
      FindAndCacheMessageHandler(*msg, *controller, &inspection_result);
  if (FLARE_UNLIKELY(!handler)) {  // No one is interested in this message. (But
                                   // why would any protocol produced it then?)
    FLARE_LOG_ERROR_EVERY_SECOND(
        "Received a message of type [{}] from [{}] which is not interested by "
        "any service. The message was successfully parsed by protocol [{}].",
        GetTypeName(*msg), ctx_->remote_peer.ToString(),
        protocol->GetCharacteristics().name);
    return;
  }

  // Prepare the execution context and call user's code.
  PrepareForRpc(inspection_result, *controller, [&] {

    // Call user's code.
    auto writer = [&](auto&& m) {
      return WriteMessage(m, protocol, controller.get(),
                          kFastCallReservedContextId);
    };

    auto processing_status = handler->FastCall(&msg, writer, &call_context);
    if (processing_status == StreamService::ProcessingStatus::Processed ||
        processing_status == StreamService::ProcessingStatus::Completed) {
      // Nothing.
    } else {
      call_context.status = -1;  // ...
      if (processing_status == StreamService::ProcessingStatus::Overloaded) {
        WriteOverloaded(*msg, protocol, &*controller);
      }  // No special action required for other errors.
    }

    WaitForRpcCompletion();


    // The connection should be closed ASAP. Calling `OnConnectionClosed` looks
    // weird as we're actually actively closing the connection. Anyway, that
    // callback should serve us well.
    if (processing_status == StreamService::ProcessingStatus::Completed) {
      owner_->OnConnectionClosed(ctx_->id);
    }
  });
}



void NormalConnectionHandler::ServiceOverloaded(std::unique_ptr<Message> msg,
                                                StreamProtocol* protocol,
                                                Controller* controller) {
  FLARE_LOG_WARNING("Server overloaded. Message is dropped.");

  WriteOverloaded(*msg, protocol, controller);
}

inline StreamService* NormalConnectionHandler::FindAndCacheMessageHandler(
    const Message& message, const Controller& controller,
    StreamService::InspectionResult* inspection_result) {
  auto last = last_service_;
  if (FLARE_LIKELY(last->Inspect(message, controller, inspection_result))) {
    return last;
  }
  return FindAndCacheMessageHandlerSlow(
      message, controller, inspection_result);  // Update `last_service_`.
}

StreamService* NormalConnectionHandler::FindAndCacheMessageHandlerSlow(
    const Message& message, const Controller& controller,
    StreamService::InspectionResult* inspection_result) {
  for (auto&& e : ctx_->services) {
    if (e->Inspect(message, controller, inspection_result)) {
      last_service_.store(e);
      return e;
    }
  }
  return nullptr;
}

template <class F>
void NormalConnectionHandler::PrepareForRpc(
    const StreamService::InspectionResult& inspection_result,
    const Controller& controller, F&& cb) {
      
    std::forward<F>(cb)();

}

void NormalConnectionHandler::WaitForRpcCompletion() {}


bool NormalConnectionHandler::OnNewCall() {
  if (!owner_->OnNewCall()) {
    return false;
  }
  ongoing_requests_.fetch_add(1);
  return true;
}

void NormalConnectionHandler::OnCallCompletion() {
  owner_->OnCallCompletion();
  // We must notify owner first, since we're waiting for our own counter to be
  // zero in `Join()`.

  // Pairs with reading of it in `Join()`.
  FLARE_CHECK_GT(ongoing_requests_.fetch_sub(1), 0);
}

}  
