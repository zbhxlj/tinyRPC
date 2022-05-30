#include "rpcControllerServer.h"

#include <optional>
#include <string>

#include "google/protobuf/empty.pb.h"

#include "../../../base/Logging.h"

namespace tinyRPC {

void RpcServerController::SetFailed(const std::string& reason) {
  return SetFailed(rpc::STATUS_FAILED, reason);
}

void RpcServerController::SetFailed(int status, std::string reason) {
  FLARE_CHECK_NE(status, rpc::STATUS_SUCCESS,
                 "You should never call `SetFailed` with `STATUS_SUCCESS`.");
  // I think we should use negative status code to represent severe errors (and
  // therefore, should be reported to NSLB.).
  //
  // Both framework-related error, and critical user error (e.g., inconsistent
  // data) can use negative errors.
  FLARE_CHECK_GE(status, 0, "Negative status codes are reserved.");
  FLARE_LOG_ERROR_IF_ONCE(
      // `STATUS_FAILED` is explicitly allowed for user to use.
      status <= rpc::STATUS_RESERVED_MAX && status != rpc::STATUS_FAILED,
      "`status` in range [0, 1000] is reserved by the framework. You should "
      "always call `SetFailed` with a status code greater than 1000.");
  error_code_ = status;
  error_text_ = std::move(reason);

}


void RpcServerController::Reset() {
  RpcControllerCommon::Reset();

  error_code_ = rpc::STATUS_SUCCESS;
  timeout_from_caller_ = std::nullopt;
  early_write_resp_cb_ = nullptr;
  error_text_.clear();
}

bool RpcServerController::Failed() const {
  return error_code_ != rpc::STATUS_SUCCESS;
}

int RpcServerController::ErrorCode() const { return error_code_; }

std::string RpcServerController::ErrorText() const { return error_text_; }

}  // namespace tinyRPC
