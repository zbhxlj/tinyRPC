#ifndef _SRC_RPC_PROTOCOL_PROTOBUF_SERVICE_H_
#define _SRC_RPC_PROTOCOL_PROTOBUF_SERVICE_H_

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "google/protobuf/service.h"
#include "gtest/gtest_prod.h"

#include "../../../base/ScopedDeferred.h"
#include "../../../base/MaybeOwning.h"
#include "../StreamService.h"

namespace tinyRPC {

class RpcServerController;

}  // namespace tinyRPC

namespace tinyRPC::protobuf {

struct ProtoMessage;

// This class acts as an adaptor from `google::protobuf::Service` to
// `flare::StreamService`.
//
// This class is also responsible for registering factories for messages used by
// its `impl`'s methods (via `MethodDescriber::Register`).
class Service : public StreamService {  // Only `StreamService` is supported
                                        // (for now).
 public:
  ~Service();

  void AddService(MaybeOwning<google::protobuf::Service> impl);

  bool Inspect(const Message& message, const Controller& controller,
               InspectionResult* result) override;

  ProcessingStatus FastCall(
      std::unique_ptr<Message>* request,
      const UniqueFunction<std::size_t(const Message&)>& writer,
      Context* context) override;


  void Stop() override;
  void Join() override;

 private:
  FRIEND_TEST(ServiceFastCallTest, RejectedDelayedFastCall);
  FRIEND_TEST(ServiceFastCallTest, AcceptedDelayedFastCall);
  FRIEND_TEST(ServiceFastCallTest, RejectedMaxOngoingFastCall);
  FRIEND_TEST(ServiceFastCallTest, AcceptedMaxOngoingFastCall);
  FRIEND_TEST(ServiceFastCallTest, MaxOngoingFlag);

  struct alignas(64) AlignedInt {
    std::atomic<int> value{};
  };

  struct MethodDesc {
    google::protobuf::Service* service;
    const google::protobuf::MethodDescriptor* method;
    const google::protobuf::Message* request_prototype;  // For doing dry-run.
    const google::protobuf::Message* response_prototype;
    std::chrono::nanoseconds max_queueing_delay{
        std::chrono::nanoseconds::max()};
    std::uint32_t max_ongoing_requests;

    // Applicable only `max_ongoing_request` is not 0.
    std::unique_ptr<AlignedInt> ongoing_requests;
  };

  // Returns [nullptr, nullptr] if the request is rejected.
  const MethodDesc* SanityCheckOrRejectEarlyForFastCall(
      const Message& msg,
      const UniqueFunction<std::size_t(const Message&)>& resp_writer,
      const Context& ctx) const;

  void InitializeServerControllerForFastCall(const ProtoMessage& msg,
                                             const Context& ctx,
                                             RpcServerController* ctlr);

  // Call user's service implementation. The response is written out by this
  // method, *and* saved into `resp_msg`.
  //
  // The reason why we don't delay response writeout until this method's
  // completes is for better responsiveness.
  void InvokeUserMethodForFastCall(
      const MethodDesc& method, const ProtoMessage& req_msg,
      ProtoMessage* resp_msg, RpcServerController* ctlr,
      const UniqueFunction<std::size_t(const Message&)>& writer, Context* ctx);


  // Determines if we have the resource to process the requested method. An
  // empty object is returned if the request should be rejected.
  Deferred AcquireProcessingQuotaOrReject(const ProtoMessage& msg,
                                          const MethodDesc& method,
                                          const Context& ctx);

  void CreateNativeResponse(const MethodDesc& method_desc,
                            const ProtoMessage& request,
                            std::unique_ptr<google::protobuf::Message> resp_ptr,
                            RpcServerController* ctlr, ProtoMessage* response);
  const MethodDesc* FindHandler(const std::string& method_name) const;

 private:
  std::vector<MaybeOwning<google::protobuf::Service>> services_;

  // Used for detecting "Service not found" error. This is only used on
  // error-path.
  //
  // Elements here references string of `ServiceDescriptor`.
  std::unordered_set<std::string_view> registered_services_;

  // This is to workaround a common misuse: Users tend to free their service
  // class before destroying `flare::Server` (and the `protobuf::Service` here).
  // By the time we're destroyed, objects referenced by `services_` may have
  // already gone. Fortunately, even if user's service class has gone, their
  // descriptors are long-lived. Therefore, we make a copy here for
  // unregistration..
  std::vector<const google::protobuf::ServiceDescriptor*> service_descs_;

  // Keyed by `MethodDescriptor::full_name()`.
  std::unordered_map<std::string, MethodDesc> method_descs_;
};

}  // namespace tinyRPC::protobuf

#endif  
