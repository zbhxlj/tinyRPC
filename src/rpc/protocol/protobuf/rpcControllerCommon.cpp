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

void RpcControllerCommon::StartCancel() {
  // Cancellation is not implemented yet.
  FLARE_CHECK(!"Not supported.");
}

bool RpcControllerCommon::IsCanceled() const {
  // Cancellation is not implemented yet.
  FLARE_CHECK(!"Not supported.");
}

void RpcControllerCommon::NotifyOnCancel(google::protobuf::Closure* callback) {
  // Cancellation is not implemented yet.
  FLARE_CHECK(!"Not supported.");
}

RpcControllerCommon::~RpcControllerCommon() {}

}  // namespace tinyRPC::protobuf
