#define _SRC_RPC_CHANNEL_SUPPRESS_INCLUDE_WARNING

#include "rpcChannel.h"

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gflags/gflags.h"

#include "../../../base/Callback.h"
#include "../../../base/Endian.h"
#include "../../../base/Function.h"
#include "../../../base/Endpoint.h"
#include "../../../base/Random.h"
#include "../../../base/String.h"
#include "../../../base/Tsc.h"
#include "../../../fiber/Latch.h"
#include "../../internal/CorrelationID.h"
#include "../../internal/StreamCallGate.h"
#include "../../MessageDispatcherFactory.h"
#include "CallContext.h"
#include "Message.h"
#include "rpcControllerClient.h"
#include "rpc_meta.pb.h"
#include "ServiceMethodLocator.h"
#include "../../internal/StreamCallGateHandle.h"
#include "MockChannel.h"

DEFINE_int32(flare_rpc_channel_max_packet_size, 4 * 1024 * 1024,
             "Default maximum packet size of `RpcChannel`.");

using namespace std::literals;

namespace tinyRPC {

template <class T>
using Factory = UniqueFunction<std::unique_ptr<T>()>;

namespace {

template <class F>
rpc::internal::StreamCallGateHandle GetOrCreateExclusive(
    const Endpoint& key, F&& creator) {
  auto rc = creator();
  FLARE_CHECK(rc->GetEndpoint() == key);
  return rpc::internal::StreamCallGateHandle(std::move(rc));
}

 struct FastCallContext {
  std::uintptr_t nslb_ctx{};
  std::shared_ptr<protobuf::ProactiveCallContext> call_ctx;
  rpc::internal::StreamCallGateHandle call_gate_handle;
};

// TODO:
protobuf::detail::MockChannel* mock_channel;

bool IsMockAddress(const std::string& address) {
  return StartsWith(address, "mock://");
}

// Returns: scheme, address.
std::optional<std::pair<std::string, std::string>> InspectUri(
    const std::string& uri) {
  static constexpr auto kSep = "://"sv;

  auto colon = uri.find(':');
  if (colon == std::string::npos) {
    return std::nullopt;
  }
  if (uri.size() < colon + kSep.size() ||
      uri.substr(colon, kSep.size()) != kSep) {
    return std::nullopt;
  }
  auto scheme = uri.substr(0, colon);
  auto address = uri.substr(colon + kSep.size());
  return std::pair(scheme, address);
}

std::unique_ptr<MessageDispatcher> NewMessageDispatcherFromName(
    std::string_view name) {
  if (auto pos = name.find_first_of('+'); pos != std::string_view::npos) {
    return MakeCompositedMessageDispatcher(name.substr(0, pos),
                                           name.substr(pos + 1));
  } else {
    return message_dispatcher_registry.TryNew(name);
  }
}

// `Random()` does not perform quite well, besides, we don't need a "real"
// random number for NSLB either, so we use a thread-local RR ID for default
// NSLB key.
std::uint64_t GetNextPseudoRandomKey() {
  thread_local std::uint64_t index = Random();
  return index++;
}

rpc::Status TranslateRpcError(rpc::internal::StreamCallGate::CompletionStatus status) {
  FLARE_CHECK(status != rpc::internal::StreamCallGate::CompletionStatus::Success);
  if (status == rpc::internal::StreamCallGate::CompletionStatus::IoError) {
    return rpc::STATUS_IO_ERROR;
  } else if (status == rpc::internal::StreamCallGate::CompletionStatus::Timeout) {
    return rpc::STATUS_TIMEOUT;
  } else if (status == rpc::internal::StreamCallGate::CompletionStatus::ParseError) {
    return rpc::STATUS_MALFORMED_DATA;
  }
  FLARE_UNREACHABLE();
}

}  // namespace

struct RpcChannel::RpcCompletionDesc {
  int status;  // Not significant if `msg` is provided. In this case
               // `msg.meta.response_meta.status` should be used instead.
  protobuf::ProtoMessage* msg = nullptr;

  const Endpoint* remote_peer = nullptr;
};

struct RpcChannel::Impl {
  // If this, this channel will be used instead. Used for performing RPC mock /
  // dry-run.
  MaybeOwning<google::protobuf::RpcChannel> alternative_channel;

  bool opened = false;
  std::unique_ptr<MessageDispatcher> message_dispatcher;
  Factory<StreamProtocol> protocol_factory;
};

RpcChannel::RpcChannel() { impl_ = std::make_unique<Impl>(); }

RpcChannel::~RpcChannel() = default;

RpcChannel::RpcChannel(std::string address, const Options& options)
    : RpcChannel() {
  (void)Open(std::move(address), options);  // Failure is ignored.
}

bool RpcChannel::Open(std::string address, const Options& options) {
  options_ = options;
  address_ = address;

  if (FLARE_UNLIKELY(IsMockAddress(address))) {
    FLARE_CHECK(mock_channel,
                "Mock channel has not been registered yet. Did you forget to "
                "link `flare/testing:rpc_mock`?");
    impl_->alternative_channel =
        std::make_unique<protobuf::detail::MockChannelAdapter>(mock_channel);
    return true;
  }

  // Parse URI.
  auto inspection_result = InspectUri(address);
  if (!inspection_result) {
    FLARE_LOG_WARNING("URI [{}] is not recognized.", address);
    return false;
  }

  // Initialize NSLB, etc.
  auto&& [scheme, addr] = *inspection_result;
  impl_->protocol_factory =
      client_side_stream_protocol_registry.GetFactory(scheme);
  if (!options.override_nslb.empty()) {
    impl_->message_dispatcher =
        NewMessageDispatcherFromName(options.override_nslb);
  } else {
    impl_->message_dispatcher = MakeMessageDispatcher("rpc", address);
  }
  if (!impl_->message_dispatcher || !impl_->message_dispatcher->Open(addr)) {
    FLARE_LOG_WARNING("URI [{}] is not resolvable.", address);
    return false;
  }
  impl_->opened = true;

  return true;
}

void RpcChannel::RegisterMockChannel(protobuf::detail::MockChannel* channel) {
  FLARE_CHECK(!mock_channel, "Mock channel has already been registered");
  mock_channel = channel;
}

void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) {
  auto ctlr = static_cast<RpcClientController*>(controller);
  ctlr->PrecheckForNewRpc();

  // Let's see if we're hooked by someone else (e.g. RPC mock).
  if (impl_->alternative_channel) {
    return impl_->alternative_channel->CallMethod(method, controller, request,
                                                  response, done);
  }

  CallMethodWritingImpl(method, ctlr, request, response, done);
}

void RpcChannel::CallMethodWritingImpl(
    const google::protobuf::MethodDescriptor* method,
    RpcClientController* controller, const google::protobuf::Message* request,
    google::protobuf::Message* response, google::protobuf::Closure* done) {
  fiber::Latch latch(1);

  auto cb = [this, controller, done, response, &latch] {
    if (done) {
      done->Run();
    } else {
      latch.count_down();
    }
  };
  auto completion = tinyRPC::NewCallback(std::move(cb));

  CallMethodWithRetry(method, controller, request, response, completion,
                      controller->GetMaxRetries());
  if (!done) {  // It was a blocking call.
    latch.wait();
  }
}

void RpcChannel::CallMethodWithRetry(
    const google::protobuf::MethodDescriptor* method,
    RpcClientController* controller, const google::protobuf::Message* request,
    google::protobuf::Message* response, google::protobuf::Closure* done,
    std::size_t retries_left) {
  auto cb = [this, method, controller, request, response, done,
             retries_left](const RpcCompletionDesc& desc) mutable {
    // The RPC has failed and there's still budget for retry, let's retry then.
    if (desc.status != rpc::STATUS_SUCCESS &&
        // Not user error.
        (desc.status != rpc::STATUS_FAILED &&
         desc.status <= rpc::STATUS_RESERVED_MAX) &&
        retries_left != 1) {
      FLARE_CHECK_GT(retries_left, 1);
      CallMethodWithRetry(method, controller, request, response, done,
                          retries_left - 1);
      return;
    }

    // It's the final result.
    controller->SetCompletion(done);
    CopyInterestedFieldsFromMessageToController(desc, controller);

    if (desc.msg) {
      auto&& resp_meta = desc.msg->meta->response_meta();
      controller->NotifyCompletion(
          FLARE_LIKELY(resp_meta.status() == rpc::STATUS_SUCCESS)
              ? Status()
              : Status(resp_meta.status(), resp_meta.description()));
    } else {
      controller->NotifyCompletion(Status(desc.status));
    }
  };
  CallMethodNoRetry(method, request, *controller, response, std::move(cb));
}

template <class F>
void RpcChannel::CallMethodNoRetry(
    const google::protobuf::MethodDescriptor* method,
    const google::protobuf::Message* request,
    const RpcClientController& controller, google::protobuf::Message* response,
    F&& cb) {
  // Find a peer to call.
  std::uintptr_t nslb_ctx;
  Endpoint remote_peer;
  if (FLARE_UNLIKELY(!GetPeerOrFailEarlyForFastCall(*method, &remote_peer,
                                                    &nslb_ctx, cb))) {
    return;
  }

  // Describe several aspect of this RPC.
  auto call_ctx = std::make_shared<protobuf::ProactiveCallContext>();
  auto call_ctx_ptr = call_ctx.get();  // Used later.
  call_ctx->method = method;
  call_ctx->response_ptr = response;

  // Open a gate and keep an extra ref. on it.
  //
  // The extra ref. is required to keep the gate alive until `FastCall`
  // finishes. This is necessary to prevent the case when our callback is called
  // (therefore, `ctlr.DetachGate()` is called) before `FastCall` return, we
  // need to keep a ref-count ourselves.
  auto gate_handle = GetFastCallGate(remote_peer);

  // Context passed to our completion callback.
  auto cb_ctx = std::make_shared<FastCallContext>();
  cb_ctx->nslb_ctx = nslb_ctx;
  cb_ctx->call_ctx = std::move(call_ctx);
  cb_ctx->call_gate_handle = std::move(gate_handle);

  // Completion callback.
  auto on_completion = [this, cb_ctx = std::move(cb_ctx), cb = std::move(cb)](
                           auto status, auto msg_ptr,
                           auto&& timestamps) mutable {
    Endpoint remote_peer = cb_ctx->call_gate_handle->GetEndpoint();

    // The RPC timed out. Besides, the connection does not support multiplexing.
    //
    // In this case we must close the connection to avoid confusion in
    // correspondence between subsequent request and pending responses (to this
    // one, and to newer request).
    if (!msg_ptr) {
      cb_ctx->call_gate_handle->SetUnhealthy();
    }
    cb_ctx->call_gate_handle.Close();

    auto proto_msg = dynamic_cast<protobuf::ProtoMessage*>(msg_ptr.get());
    int rpc_status = proto_msg ? proto_msg->meta->response_meta().status()
                               : TranslateRpcError(status);

    cb(RpcCompletionDesc{.status = rpc_status,
                         .msg = proto_msg,
                         .remote_peer = &remote_peer});
  };

  // Prepare the request message.
  protobuf::ProtoMessage req_msg;
  CreateNativeRequestForFastCall(*method, request, controller, &req_msg);

  // And issue the call.
  auto args = std::make_shared<rpc::internal::StreamCallGate::FastCallArgs>();
  args->completion = std::move(on_completion);
  args->controller = call_ctx_ptr;
  gate_handle->FastCall(req_msg, std::move(args), controller.GetTimeout());
}


template <class F>
bool RpcChannel::GetPeerOrFailEarlyForFastCall(
    const google::protobuf::MethodDescriptor& method, Endpoint* peer,
    std::uintptr_t* nslb_ctx, F&& cb) {
  if (FLARE_UNLIKELY(!impl_->opened)) {
    FLARE_LOG_WARNING(
        "Calling method [{}] on failed channel [{}].", method.full_name(),
        address_);
    cb(RpcCompletionDesc{.status = rpc::STATUS_INVALID_CHANNEL});
    return false;
  }
  if (FLARE_UNLIKELY(!impl_->message_dispatcher->GetPeer(
          GetNextPseudoRandomKey(), peer, nslb_ctx))) {
    FLARE_LOG_WARNING(
        "No peer available for calling method [{}] on [{}].",
        method.full_name(), address_);
    cb(RpcCompletionDesc{.status = rpc::STATUS_NO_PEER});
    return false;
  }

  return true;
}

void RpcChannel::CreateNativeRequestForFastCall(
    const google::protobuf::MethodDescriptor& method,
    const google::protobuf::Message* request,
    const RpcClientController& controller, protobuf::ProtoMessage* to) {
  // Initialize meta.
  auto meta = std::make_shared<rpc::RpcMeta>();
  meta->set_method_type(rpc::METHOD_TYPE_SINGLE);
  meta->mutable_request_meta()->mutable_method_name()->assign(
      method.full_name().begin(), method.full_name().end());
  meta->mutable_request_meta()->set_timeout(controller.GetRelativeTimeout() /
                                            1ms);
  to->meta = std::move(meta);

  // Initialize body.
  to->msg = MaybeOwning(non_owning, request);

  // And (optionally) the attachment.
  to->attachment = controller.GetRequestAttachment();
}

rpc::internal::StreamCallGateHandle RpcChannel::GetFastCallGate(
    const Endpoint& ep) {
    return GetOrCreateExclusive(
        ep, [&] { return CreateCallGate(ep); });
  }


std::shared_ptr<rpc::internal::StreamCallGate> RpcChannel::CreateCallGate(
    const Endpoint& ep) {
  auto gate = std::make_shared<rpc::internal::StreamCallGate>();
  rpc::internal::StreamCallGate::Options opts;
  opts.protocol = impl_->protocol_factory();
  opts.maximum_packet_size = options_.maximum_packet_size;
  gate->Open(ep, std::move(opts));
  if (!gate->Healthy()) {
    FLARE_LOG_WARNING("Failed to open new call gate to [{}].",
                                   ep.ToString());
    // Fall-through. We don't want to handle failure in any special way.
  }
  return gate;
}

void RpcChannel::CopyInterestedFieldsFromMessageToController(
    const RpcCompletionDesc& completion_desc, RpcClientController* ctlr) {
  ctlr->SetRemotePeer(*completion_desc.remote_peer);
  if (auto msg = completion_desc.msg) {
    ctlr->SetResponseAttachment(msg->attachment);
  }
}

}  // namespace tinyRPC
