#include "rpcControllerClient.h"

#include <chrono>
#include <string>

#include "gflags/gflags.h"

#include "../../../base/Logging.h"

using namespace std::literals;

DEFINE_int32(flare_rpc_client_default_rpc_timeout_ms, 2000,
             "Default RPC timeout for non-streaming RPCs.");

namespace tinyRPC {

bool RpcClientController::Failed() const {
  FLARE_CHECK(
      completed_,
      "Calling `Failed()` before RPC has completed makes no sense. If you see "
      "this error in UT, it's likely your RPC mock does not work correctly.");
  return !rpc_status_ || !rpc_status_->ok();
}

int RpcClientController::ErrorCode() const {
  return rpc_status_ ? rpc_status_->code()
                     : static_cast<int>(rpc::STATUS_FAILED);
}

std::string RpcClientController::ErrorText() const {
  if (!rpc_status_) {
    return "(unknown failure)";
  }
  return rpc_status_->message();
}


void RpcClientController::SetMaxRetries(std::size_t max_retries) {
  max_retries_ = max_retries;
}

std::size_t RpcClientController::GetMaxRetries() const { return max_retries_; }


void RpcClientController::SetFailed(const std::string& reason) {
  FLARE_CHECK(0, "Unexpected.");
}


void RpcClientController::Reset() {
  RpcControllerCommon::Reset();
  in_use_ = false;
  completed_ = false;

  max_retries_ = 1;
  last_reset_ = ReadSteadyClock();
  timeout_ = last_reset_ + 1ms * FLAGS_flare_rpc_client_default_rpc_timeout_ms;

  completion_ = nullptr;

  rpc_status_ = std::nullopt;
}

void RpcClientController::PrecheckForNewRpc() {
  FLARE_LOG_ERROR_IF(
      in_use_,
      "UNDEFINED BEHAVIOR: You must `Reset()` the `RpcClientController` before "
      "reusing it. THIS ERROR WILL BE RAISED TO A CHECK FAILURE (CRASHING THE "
      "PROGRAM) SOON.");
  FLARE_DCHECK(!in_use_);
  in_use_ = true;
}

void RpcClientController::SetCompletion(google::protobuf::Closure* done) {
  completion_ = std::move(done);
}

void RpcClientController::NotifyCompletion(Status status) {
  rpc_status_ = status;

  completed_ = true;
  completion_->Run();

  // Do NOT touch this controller hereafter, as it could have been destroyed in
  // user's completion callback.
}

}  // namespace tinyRPC
