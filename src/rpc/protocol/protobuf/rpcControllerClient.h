#ifndef _SRC_RPC_PROTOCOL_PROTOBUF_RPC_CLIENT_CONTROLLER_H_
#define _SRC_RPC_PROTOCOL_PROTOBUF_RPC_CLIENT_CONTROLLER_H_

#include <string>
#include <optional>

#include "gflags/gflags_declare.h"
#include "gtest/gtest_prod.h"

#include "../../../base/Status.h"
#include "rpcControllerCommon.h"

DECLARE_int32(flare_rpc_client_default_rpc_timeout_ms);

namespace tinyRPC {

namespace testing::detail {

class MockRpcChannel;

}  // namespace testing::detail


// This controller controls a single RPC. It's used on client-side.
class RpcClientController : public protobuf::RpcControllerCommon {
 public:
  RpcClientController() : protobuf::RpcControllerCommon(false) {}

  // Test if the call failed.
  //
  // It makes no sense to call this method before RPC is completed.
  bool Failed() const override;

  // Returns error code of this call, or `STATUS_SUCCESS` if no failure
  // occurred.
  int ErrorCode() const override;  // @sa: `Status` in `rpc_meta.proto`.

  // Returns whatever the server used to describe the error.
  std::string ErrorText() const override;
  
  std::chrono::steady_clock::time_point GetTimeout() const noexcept;

  // Make sure that your call is idempotent before enabling this.
  //
  // If not set, 1 is the default (i.e., no retry).
  //
  // Note that this method has no effect on streaming RPC.
  void SetMaxRetries(std::size_t max_retries);
  std::size_t GetMaxRetries() const;

  // For normal RPCs, certain protocols allow you to send an "attachment" along
  // with the message, which is more efficient compared to serializing the
  // attachment into the message.
  using RpcControllerCommon::GetRequestAttachment;
  using RpcControllerCommon::GetResponseAttachment;
  using RpcControllerCommon::SetRequestAttachment;

  // This allows you to know who is requesting you.
  using RpcControllerCommon::GetRemotePeer;


  // Reset this controller to its initial status.
  //
  // If you reuse the controller for successive RPCs, you must call `Reset()` in
  // between. We cannot do this for you, as it's only after your `done` has been
  // called, the controller can be `Reset()`-ed. But after your `done` is
  // called, it's possible that the controller itself is destroyed (presumably
  // by you), and by that time, to avoid use-after-free, we cannot `Reset()` the
  // controller.
  void Reset() override;

 private:  // We need package visibility here.
  FRIEND_TEST(RpcClientController, Basics);
  friend class RpcChannel;  // It needs access to several private methods below.
  friend class testing::detail::MockRpcChannel;
  friend struct testing::detail::RpcControllerMaster;

  // Make sure the controller is NOT in used and mark it as being used.
  void PrecheckForNewRpc();

  // Get timeout in relative (to `last_reset_`) time.
  std::chrono::nanoseconds GetRelativeTimeout() const noexcept {
    return timeout_ - last_reset_;
  }


  // `done` is called upon RPC completion.
  void SetCompletion(google::protobuf::Closure* done);

  // Notifies RPC completion. This method is only used for fast calls.
  //
  // If no response is received, the overload with manually-provided status is
  // called.
  void NotifyCompletion(Status status);

  // Not used.
  void SetFailed(const std::string& reason) override;

 private:
  bool in_use_ = false;
  bool completed_ = false;

  // User settings.
  std::size_t max_retries_ = 1;
  std::chrono::steady_clock::time_point last_reset_{ReadSteadyClock()};
  std::chrono::steady_clock::time_point timeout_{
      last_reset_ +
      std::chrono::milliseconds(FLAGS_flare_rpc_client_default_rpc_timeout_ms)};

  google::protobuf::Closure* completion_ = nullptr;

  // RPC State.
  std::optional<Status> rpc_status_;
};

}  // namespace tinyRPC

#endif  
