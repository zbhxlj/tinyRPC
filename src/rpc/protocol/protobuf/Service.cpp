#define FLARE_RPC_SERVER_CONTROLLER_SUPPRESS_INCLUDE_WARNING

#include "Service.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include "gflags/gflags.h"
#include "google/protobuf/descriptor.h"

#include "../../../base/Callback.h"
#include "../../../base/String.h"
#include "../../../base/Latch.h"
#include "CallContext.h"
#include "rpcControllerServer.h"
#include "ServiceMethodLocator.h"
#include "../StreamProtocol.h"
#include "../../rpc_options.pb.h"

using namespace std::literals;

DEFINE_string(flare_rpc_server_protocol_buffers_max_ongoing_requests_per_method,
              "",
              "If set, a list of method_full_name:limit, separated by comma, "
              "should be provided. This flag controls allowed maximum "
              "concurrent requests, in a per-method fashion. e.g.: "
              "`flare.example.EchoService.Echo:10000,flare.example.EchoService."
              "Echo2:5000`. If both this option and Protocol Buffers option "
              "`flare.max_ongoing_requests` are applicable, the smaller one "
              "is respected.");

namespace tinyRPC::protobuf {

namespace {

ProtoMessage CreateErrorResponse(std::uint64_t correlation_id,
                                 rpc::Status status, std::string description) {
  FLARE_CHECK(status != rpc::STATUS_SUCCESS);
  auto meta = std::make_shared<rpc::RpcMeta>();
  meta->set_correlation_id(correlation_id);
  meta->set_method_type(rpc::METHOD_TYPE_SINGLE);
  meta->mutable_response_meta()->set_status(status);
  meta->mutable_response_meta()->set_description(std::move(description));
  return ProtoMessage(std::move(meta), nullptr);
}

std::unordered_map<std::string, std::uint32_t> ParseMaxOngoingRequestFlag() {
  std::unordered_map<std::string, std::uint32_t> result;

  auto splitted = Split(
      FLAGS_flare_rpc_server_protocol_buffers_max_ongoing_requests_per_method,
      ",");
  for (auto&& e : splitted) {
    auto method_limit = Split(e, ":");
    FLARE_CHECK(method_limit.size() == 2,
                "Invalid per-method max-ongoing-requests config: [{}]", e);
    auto name = std::string(method_limit[0]);
    auto method_desc =
        google::protobuf::DescriptorPool::generated_pool()->FindMethodByName(
            name);
    FLARE_CHECK(method_desc, "Unrecognized method [{}].", method_limit[0]);
    auto limit = TryParse<std::uint32_t>(method_limit[1]);
    FLARE_CHECK(limit, "Invalid max-ongoing-request limit [{}].",
                method_limit[1]);
    result[name] = *limit;
  }
  return result;
}

}  // namespace

Service::~Service() {
  for (auto&& e : service_descs_) {
    ServiceMethodLocator::Instance()->DeleteService(e);
  }
}

void Service::AddService(MaybeOwning<google::protobuf::Service> impl) {
  static const auto kMaxOngoingRequestsConfigs = ParseMaxOngoingRequestFlag();

  auto&& service_desc = impl->GetDescriptor();

  for (int i = 0; i != service_desc->method_count(); ++i) {
    auto method = service_desc->method(i);
    auto name = method->full_name();

    FLARE_CHECK(method_descs_.find(name) == method_descs_.end(),
                "Duplicate method: {}", name);
    auto&& e = method_descs_[name];

    // Basics.
    e.service = impl.Get();
    e.method = method;
    e.request_prototype =
        google::protobuf::MessageFactory::generated_factory()->GetPrototype(
            method->input_type());
    e.response_prototype =
        google::protobuf::MessageFactory::generated_factory()->GetPrototype(
            method->output_type());

    // Limit on maximum delay in dispatch queue.
    if (auto delay =
            method->options().GetExtension(flare::max_queueing_delay_ms)) {
      e.max_queueing_delay = 1ms * delay;
    }

    // Limit on maximum concurency.
    e.max_ongoing_requests = std::numeric_limits<std::uint32_t>::max();
    if (method->options().HasExtension(flare::max_ongoing_requests)) {
      e.max_ongoing_requests =
          method->options().GetExtension(flare::max_ongoing_requests);
    }
    if (auto iter = kMaxOngoingRequestsConfigs.find(name);
        iter != kMaxOngoingRequestsConfigs.end()) {
      e.max_ongoing_requests = std::min(e.max_ongoing_requests, iter->second);
    }
    if (e.max_ongoing_requests != std::numeric_limits<std::uint32_t>::max()) {
      e.ongoing_requests = std::make_unique<AlignedInt>();
    }

  }

  services_.push_back(std::move(impl));
  ServiceMethodLocator::Instance()->AddService(
      services_.back()->GetDescriptor());
  service_descs_.push_back(services_.back()->GetDescriptor());
  registered_services_.insert(services_.back()->GetDescriptor()->full_name());
}

bool Service::Inspect(const Message& message, const Controller& controller,
                      InspectionResult* result) {
  if (auto msg = dynamic_cast<const ProtoMessage*>(&message); FLARE_LIKELY(msg)) {
    result->method = msg->meta->request_meta().method_name();
    return true;
  } else if (dynamic_cast<const EarlyErrorMessage*>(&message)) {
    result->method = "(unrecognized method)";
    return true;  // `result->method` is not filled. We don't recognize the
                  // method being called anyway.
  }
  return false;
}


Service::ProcessingStatus Service::FastCall(
    std::unique_ptr<Message>* request,
    const UniqueFunction<std::size_t(const Message&)>& writer, Context* context) {
  // Do some sanity check first.
  auto method_desc =
      SanityCheckOrRejectEarlyForFastCall(**request, writer, *context);
  if (FLARE_UNLIKELY(!method_desc)) {
    return ProcessingStatus::Processed;
  }

  auto req_msg = dynamic_cast<ProtoMessage*>(&**request);
  auto processing_quota =
      AcquireProcessingQuotaOrReject(*req_msg, *method_desc, *context);
  if (!processing_quota) {
    return ProcessingStatus::Overloaded;
  }

  // Initialize server RPC controller.
  RpcServerController rpc_controller;
  InitializeServerControllerForFastCall(*req_msg, *context, &rpc_controller);

  // Call user's implementation and sent response out.
  ProtoMessage resp_msg;
  InvokeUserMethodForFastCall(*method_desc, *req_msg, &resp_msg,
                              &rpc_controller, writer, context);

  return ProcessingStatus::Processed;
}

void Service::Stop() {
  // Nothing.
  //
  // Outstanding requests are counted by `Server`, we don't have to bother doing
  // that.)
}

void Service::Join() {
  // Nothing.
}

const Service::MethodDesc* Service::SanityCheckOrRejectEarlyForFastCall(
    const Message& msg,
    const UniqueFunction<std::size_t(const Message&)>& resp_writer,
    const Context& ctx) const {
  auto msg_ptr = dynamic_cast<const ProtoMessage*>(&msg);
  if (FLARE_UNLIKELY(!msg_ptr)) {
    auto e = dynamic_cast<const EarlyErrorMessage*>(&msg);
    FLARE_CHECK(e);  // Otherwise either the framework or `Inspect` is
    // misbehaving.
    resp_writer(CreateErrorResponse(e->GetCorrelationId(), e->GetStatus(),
                                    e->GetDescription()));
    return nullptr;
  }

  // Otherwise our protocol lack some basic sanity checks.
  FLARE_CHECK(msg_ptr->meta->has_request_meta());

  // Note that even if our protocol object recognizes the method, it's possible
  // that the service the method belongs to, is not registered with us. If the
  // server is serving different "service" on different port, this can be the
  // case.
  auto&& method_name = msg_ptr->meta->request_meta().method_name();
  auto&& method_desc = FindHandler(method_name);
  if (FLARE_UNLIKELY(!method_desc)) {
    std::string_view service_name = method_name;
    if (auto pos = service_name.find_last_of('.');
        pos != std::string_view::npos) {
      service_name = service_name.substr(0, pos);
    }
    if (registered_services_.count(service_name) == 0) {
      resp_writer(CreateErrorResponse(
          msg_ptr->GetCorrelationId(), rpc::STATUS_SERVICE_NOT_FOUND,
          Format("Service [{}] is not found.", service_name)));
    } else {
      resp_writer(CreateErrorResponse(
          msg_ptr->GetCorrelationId(), rpc::STATUS_METHOD_NOT_FOUND,
          Format("Method [{}] is not found.", method_name)));
    }
    return nullptr;
  }

  return method_desc;
}

void Service::InitializeServerControllerForFastCall(const ProtoMessage& msg,
                                                    const Context& ctx,
                                                    RpcServerController* ctlr) {
  ctlr->SetRemotePeer(ctx.remote_peer);

  if (auto v = msg.meta->request_meta().timeout()) {
    // `received_tsc` is the most accurate timestamp we can get. However, there
    // can still be plenty of time elapsed on the network.
    ctlr->SetTimeout(TimestampFromTsc(ctx.received_tsc) + v * 1ms);
  }
  if (FLARE_UNLIKELY(!msg.attachment.empty())) {
    ctlr->SetRequestAttachment(msg.attachment);
  }

}

void Service::InvokeUserMethodForFastCall(
    const MethodDesc& method, const ProtoMessage& req_msg,
    ProtoMessage* resp_msg, RpcServerController* ctlr,
    const UniqueFunction<std::size_t(const Message&)>& writer, Context* ctx) {
  // Prepare response message.
  auto resp_ptr = std::unique_ptr<google::protobuf::Message>(
      method.response_prototype->New());

  // For better responsiveness, we allow the user to write response early via
  // `RpcServerController::WriteResponseImmediately` (or, if not called, once
  // `done` is called, so we have to provide a callback to fill and write the
  // response.
  internal::LocalCallback write_resp_callback([&] {
    CreateNativeResponse(method, req_msg, std::move(resp_ptr), ctlr, resp_msg);

    // Note that `writer` does not mutate `resp_msg`, we rely on this as we'll
    // still need the response later.
    writer(*resp_msg);
  });
  ctlr->SetEarlyWriteResponseCallback(&write_resp_callback);

  // We always call the callback in a synchronous fashion. Given that our fiber
  // runtime is fairly fast, there's no point in implementing method invocation
  // in a "asynchronous" fashion, which is even slower.
  tinyRPC::Latch done_latch(1);
  internal::LocalCallback done_callback([&] {
    // If the user did not call `WriteResponseImmediately` (likely), let's call
    // it for them.
    if (auto ptr = ctlr->DestructiveGetEarlyWriteResponse()) {
      ptr->Run();
    }
    done_latch.count_down();
  });

  method.service->CallMethod(method.method, ctlr, req_msg.msg.value().Get(), resp_ptr.get(), &done_callback);
  done_latch.wait();

  // Save the result for later use.
  ctx->status = ctlr->ErrorCode();
}



Deferred Service::AcquireProcessingQuotaOrReject(const ProtoMessage& msg,
                                                 const MethodDesc& method,
                                                 const Context& ctx) {
  auto max_queueing_delay = method.max_queueing_delay;
  if (auto v = msg.meta->request_meta().timeout()) {
    max_queueing_delay =
        std::min<std::chrono::nanoseconds>(max_queueing_delay, v * 1ms);
  }
  if (FLARE_UNLIKELY(DurationFromTsc(ctx.received_tsc, ReadTsc()) >
                     max_queueing_delay)) {
    FLARE_LOG_WARNING(
        "Rejecting call to [{}] from [{}]: It has been in queue for too long.",
        msg.meta->request_meta().method_name(), ctx.remote_peer.ToString());
    return Deferred();
  }

  auto&& ongoing_req_ptr = method.ongoing_requests.get();
  if (FLARE_UNLIKELY(
          ongoing_req_ptr &&
          ongoing_req_ptr->value.fetch_add(1, std::memory_order_relaxed) + 1 >
              method.max_ongoing_requests)) {
    ongoing_req_ptr->value.fetch_sub(1, std::memory_order_relaxed);
    FLARE_LOG_WARNING(
        "Rejecting call to [{}] from [{}]: Too many concurrent requests.",
        msg.meta->request_meta().method_name(), ctx.remote_peer.ToString());
    return Deferred();
  }

  return Deferred([ongoing_req_ptr] {
    // Restore ongoing request counter.
    if (ongoing_req_ptr) {
      FLARE_CHECK_GE(
          ongoing_req_ptr->value.fetch_sub(1, std::memory_order_relaxed), 0);
    }
  });
}

void Service::CreateNativeResponse(
    const MethodDesc& method_desc, const ProtoMessage& request,
    std::unique_ptr<google::protobuf::Message> resp_ptr,
    RpcServerController* ctlr, ProtoMessage* response) {
  // Message meta goes first.
  auto meta = std::make_shared<rpc::RpcMeta>();
  meta->set_correlation_id(request.GetCorrelationId());
  meta->set_method_type(rpc::METHOD_TYPE_SINGLE);

  auto&& resp_meta = *meta->mutable_response_meta();
  resp_meta.set_status(ctlr->ErrorCode());
  if (FLARE_UNLIKELY(ctlr->Failed())) {
    resp_meta.set_description(ctlr->ErrorText());
  }

  response->meta = std::move(meta);

  response->msg = std::move(resp_ptr);

  // And the attachment.
  if (auto&& att = ctlr->GetResponseAttachment(); !att.empty()) {
    response->attachment = att;
  }
}

inline const Service::MethodDesc* Service::FindHandler(
    const std::string& method_name) const {
  if(method_descs_.find(method_name) != method_descs_.end()){
    return &method_descs_.at(method_name);
  }
  return nullptr;
}

}  // namespace tinyRPC::protobuf
