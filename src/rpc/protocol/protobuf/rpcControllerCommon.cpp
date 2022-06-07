#include "rpcControllerCommon.h"

#include <chrono>

#include "../../internal/StreamCallGate.h"
#include "Message.h"

using namespace std::literals;

namespace tinyRPC::protobuf {


void RpcControllerCommon::Reset() {
  request_attachment_.clear();
  response_attachment_.clear();
}

RpcControllerCommon::~RpcControllerCommon() {}

}  // namespace tinyRPC::protobuf
