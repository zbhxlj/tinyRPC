#ifndef _SRC_RPC_PROTOCOL_PROTOBUF_RPC_SERVER_CONTROLLER_H_
#define _SRC_RPC_PROTOCOL_PROTOBUF_RPC_SERVER_CONTROLLER_H_

#include <bitset>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"

#include "../../../base/String.h"
#include "rpcControllerCommon.h"

namespace tinyRPC {

namespace protobuf {

class Service;
class PassiveCallContextFactory;

}  // namespace protobuf

// This controller controls a single RPC. It's used on server-side.
class RpcServerController : public protobuf::RpcControllerCommon {
 public:
  RpcServerController() : protobuf::RpcControllerCommon(true) {}

  // Mark this RPC failed with a reason.
  //
  // Note that `status` no greater than 1000 are reserved for framework. You
  // should always choose your status code in [1001, INT32_MAX]. For the sake of
  // convenience, you're allowed to use `STATUS_FAILED` here, even if it's
  // defined by the framework.
  void SetFailed(const std::string& reason) override;  // Uses `STATUS_FAILED`.
  void SetFailed(int status, std::string reason = "");

  // Returns `true` if `SetFailed` was called.
  bool Failed() const override;

  // Returns whatever was set by `SetFailed`, or `STATUS_SUCCESS` if `SetFailed`
  // was never called. (Perhaps we should call it `GetStatus()` instead.)
  int ErrorCode() const override;  // @sa: `Status` in `rpc_meta.proto`.

  // Returns whatever was given to `SetFailed`.
  std::string ErrorText() const override;

  void SetTimeout(std::chrono::steady_clock::time_point timeout) {
    timeout_from_caller_ = timeout;
  }

  std::optional<std::chrono::steady_clock::time_point> GetTimeout() const noexcept;


  // If you're really care about responsiveness, once you have finished filling
  // out response message (but before cleaning up things in service method
  // implementation), you can call this method to write response immediately.
  //
  // ONCE THIS METHOD IS CALLED, TOUCHING RESPONSE OR CONTROLLER IN ANY FASHION
  // RESULTS IN UNDEFINED BEHAVIOR.
  //
  // This method can be called at most once. It's not applicable to streaming
  // RPCs.
  void WriteResponseImmediately();

  // No `SetTimeout` here. If you want to set a timeout for streaming RPC, use
  // `StreamReader/Writer::SetExpiration` instead.

  // For normal RPCs, certain protocols allow you to send an "attachment" along
  // with the message, which is more efficient compared to serializing the
  // attachment into the message.
  using RpcControllerCommon::GetRequestAttachment;
  using RpcControllerCommon::GetResponseAttachment;
  using RpcControllerCommon::SetResponseAttachment;

  // This allows you to know who is requesting you.
  using RpcControllerCommon::GetRemotePeer;

  // Reset this controller to its initial status.
  void Reset() override;

 private:
  FRIEND_TEST(RpcServerController, Basics);
  FRIEND_TEST(RpcServerController, Timeout);
  FRIEND_TEST(RpcServerController, Compression);

  friend class protobuf::Service;
  friend class protobuf::PassiveCallContextFactory;
  friend struct testing::detail::RpcControllerMaster;

  // @sa: `WriteResponseImmediately`.
  void SetEarlyWriteResponseCallback(
      google::protobuf::Closure* callback) noexcept {
    early_write_resp_cb_ = callback;
  }
  google::protobuf::Closure* DestructiveGetEarlyWriteResponse() {
    return std::exchange(early_write_resp_cb_, nullptr);
  }


 private:
  int error_code_ = rpc::STATUS_SUCCESS;
  // If non-empty, provide the timeout set by the caller. The protocol we're
  // using must support this field for it to be useful.
  std::optional<std::chrono::steady_clock::time_point> timeout_from_caller_;
  google::protobuf::Closure* early_write_resp_cb_ = nullptr;
  std::string error_text_;
  std::mutex user_fields_lock_;
};

/////////////////////////////////////
// Implementation goes below.      //
/////////////////////////////////////

inline void RpcServerController::WriteResponseImmediately() {
  std::exchange(early_write_resp_cb_, nullptr)->Run();
}

inline std::optional<std::chrono::steady_clock::time_point>
RpcServerController::GetTimeout() const noexcept {
  return timeout_from_caller_;
}

}  // namespace tinyRPC

#endif
