#ifndef _SRC_RPC_PROTOCOL_PROTOBUF_MOCK_CHANNEL_H_
#define _SRC_RPC_PROTOCOL_PROTOBUF_MOCK_CHANNEL_H_

#include "google/protobuf/service.h"

namespace tinyRPC::protobuf::detail {

// Interface of RPC mock channel.
class MockChannel {
 public:
  virtual ~MockChannel() = default;

  virtual void CallMethod(const MockChannel* self,
                          const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller,
                          const google::protobuf::Message* request,
                          google::protobuf::Message* response,
                          google::protobuf::Closure* done) = 0;
};

// Adapts `MockChannel` to `google::protobuf::RpcChannel` for easier use.
class MockChannelAdapter : public google::protobuf::RpcChannel {
 public:
  explicit MockChannelAdapter(MockChannel* channel) : channel_(channel) {}

  void CallMethod(const google::protobuf::MethodDescriptor* method,
                  google::protobuf::RpcController* controller,
                  const google::protobuf::Message* request,
                  google::protobuf::Message* response,
                  google::protobuf::Closure* done) override {
    return channel_->CallMethod(nullptr, method, controller, request, response,
                                done);
  }

 private:
  MockChannel* channel_;
};

}  // namespace tinyRPC::protobuf::detail

#endif  
