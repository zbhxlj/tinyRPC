#ifndef _SRC_RPC_PROTOCOL_PROTOBUF_RPC_CHANNEL_H_
#define _SRC_RPC_PROTOCOL_PROTOBUF_RPC_CHANNEL_H_

#include <memory>
#include <string>

#include "gflags/gflags_declare.h"
#include "gtest/gtest_prod.h"
#include "google/protobuf/service.h"

#include "../../../base/internal/LazyInit.h"

DECLARE_int32(flare_rpc_channel_max_packet_size);

namespace tinyRPC {

class Endpoint;
class RpcClientController;
class Message;

namespace rpc::internal {

class StreamCallGate;
class StreamCallGateHandle;

}  // namespace rpc::internal

namespace protobuf {

struct ProtoMessage;

}

// This one represents a virtual "channel" between us and the service. You may
// use this one to make RPCs to services that are using Protocol Buffers.
//
// In most cases, you should be using `Channel` via an `XxxService_Stub`
// instead of calling its methods yourself.
//
// TODO(luobogao): Given that `RpcChannel` should be made lightweight to create,
// we should support implicit channel creation like this:
//
//   // Create both a stub and (internally) a channel.
//   EchoService_SyncStub stub("flare://some-polaris-address");
class RpcChannel : public google::protobuf::RpcChannel {
 public:
  struct Options {
    // Maximum packet size.
    //
    // We won't allocate so many bytes immediately, neither won't we keep the
    // buffer such large after it has been consumed. This is only an upper limit
    // to keep you safe in case of a malfunctioning server.
    std::size_t maximum_packet_size = FLAGS_flare_rpc_channel_max_packet_size;

    // If non-empty, NSLB specified here will be used in place of the default
    // NSLB mechanism of protocol being used.
    std::string override_nslb;
  };

  RpcChannel();
  ~RpcChannel();

  // Almost the same as constructing a channel and calls `Open` on it, except
  // that failure in opening channel won't raise an error. Instead, any RPCs
  // made through the resulting channel will fail with `STATUS_INVALID_CHANNEL`.
  RpcChannel(std::string address,
             const Options& options = internal::LazyInitConstant<Options>());

  // `address` uses URI syntax (@sa: RFC 3986). NSLB is inferred from `scheme`
  // being used in `address. In general, polaris is the perferred NSLB.
  //
  // Making RPCs on a channel that haven't been `Open`ed successfully result in
  // undefined behavior.
  bool Open(std::string address,
            const Options& options = internal::LazyInitConstant<Options>());


 protected:
  // `request` / `response` is ignored if the `method` accepts a stream of
  // requests (i.e., streaming RPC is used.). In this case requests should be
  // read / written via `GetStreamReader()` / `GetStreamWriter()` of
  // `controller`.
  //
  //
  // For non-streaming call, if `done` is not provided, it's a blocking call.
  void CallMethod(const google::protobuf::MethodDescriptor* method,
                  google::protobuf::RpcController* controller,
                  const google::protobuf::Message* request,
                  google::protobuf::Message* response,
                  google::protobuf::Closure* done) override;

 private:
  struct RpcCompletionDesc;

  FRIEND_TEST(Channel, L5);

  void CallMethodWithRetry(const google::protobuf::MethodDescriptor* method,
                           RpcClientController* controller,
                           const google::protobuf::Message* request,
                           google::protobuf::Message* response,
                           google::protobuf::Closure* done,
                           std::size_t retries_left);

  // Make an RPC.
  //
  // This method is carefully designed so that you can call it concurrently even
  // with the same controller. This is essential for implementing things such as
  // backup request.
  //
  // The caller is responsible for ensuring that `response` is not shared with
  // anyone else.
  //
  // `cb` is called with `(const RpcCompletionDesc&)`.
  template <class F>
  void CallMethodNoRetry(const google::protobuf::MethodDescriptor* method,
                         const google::protobuf::Message* request,
                         const RpcClientController& controller,
                         google::protobuf::Message* response, F&& cb);


  std::uint32_t NextCorrelationId() const noexcept;

  template <class F>
  bool GetPeerOrFailEarlyForFastCall(
      const google::protobuf::MethodDescriptor& method, Endpoint* peer,
      std::uintptr_t* nslb_ctx, F&& cb);

  void CreateNativeRequestForFastCall(
      const google::protobuf::MethodDescriptor& method,
      const google::protobuf::Message* request,
      const RpcClientController& controller, protobuf::ProtoMessage* to);

  void CopyInterestedFieldsFromMessageToController(
      const RpcCompletionDesc& completion_desc, RpcClientController* ctlr);

 private:
  rpc::internal::StreamCallGateHandle GetFastCallGate(const Endpoint& ep);
  std::shared_ptr<rpc::internal::StreamCallGate> CreateCallGate(const Endpoint& ep);

 private:
  // To avoid bring in too many headers, we define our members inside `Impl`.
  struct Impl;

  Options options_;
  std::string address_;
  std::unique_ptr<Impl> impl_;
};

}  // namespace tinyRPC

#endif  
