#ifndef _SRC_RPC_PROTOCOL_PROTOBUF_RPC_CONTROLLER_COMMON_H_
#define _SRC_RPC_PROTOCOL_PROTOBUF_RPC_CONTROLLER_COMMON_H_

#include <chrono>
#include <memory>

#include "gflags/gflags_declare.h"
#include "google/protobuf/service.h"

#include "../../../base/Endpoint.h"
#include "../../../base/Tsc.h"
#include "Message.h"
#include "rpc_meta.pb.h"


namespace tinyRPC::testing::detail {

struct RpcControllerMaster;
class MockRpcChannel;

}  // namespace tinyRPC::testing::detail

namespace tinyRPC::protobuf {

// Implementation detail.
//
// Depending on whether you're writing server-side or client-side code, you
// should use `RpcServerController` or `RpcClientController` instead.
class RpcControllerCommon : public google::protobuf::RpcController {
  // Methods are selectively exposed by subclasses. Refer to them for
  // documentation.
 protected:

  explicit RpcControllerCommon(bool server_side) : server_side_(server_side) {
    Reset();
  }
  ~RpcControllerCommon();

  // If there's an attachment attached to the request / response, it's stored
  // here.
  //
  // Not all protocol supports this. If you're using attachment on a protocol
  // not supporting attachment, either `STATUS_NOT_SUPPORTED` is returned
  // (preferrably), or a warning is written to log.

  void SetRequestAttachment(std::string attachment) noexcept {
    request_attachment_ = std::move(attachment);
  }
  const std::string& GetRequestAttachment() const noexcept {
    return request_attachment_;
  }
  void SetResponseAttachment(std::string attachment) noexcept {
    response_attachment_ = std::move(attachment);
  }
  const std::string& GetResponseAttachment() const noexcept {
    return response_attachment_;
  }

  // Test if this is a server-side controller.
  bool IsServerSideController() const noexcept { return server_side_; }


  // Reset the controller.
  void Reset() override;


  // Cancellation is not implemented yet.
  void StartCancel() override;
  bool IsCanceled() const override;
  void NotifyOnCancel(google::protobuf::Closure* callback) override;

  // Get error code of this RPC, or `STATUS_SUCCESS` if no failure occurred.
  virtual int ErrorCode() const = 0;

  // Set local & remote peer address.
  void SetRemotePeer(const Endpoint& remote_peer) noexcept {
    remote_peer_ = std::move(remote_peer);
  }
  // void SetLocalPeer(Endpoint local_peer);

  // Get remote peer's address.
  //
  // Server side only.
  const Endpoint& GetRemotePeer() const noexcept { return remote_peer_; }

 private:
  friend class testing::detail::MockRpcChannel;
  friend struct testing::detail::RpcControllerMaster;
  friend class RpcChannel;

 private:
  bool server_side_;  // Never changes.
  Endpoint remote_peer_;
  // If there was an attachment attached to the request / response, it's saved
  // here.
  std::string request_attachment_;
  std::string response_attachment_;

};

}  // namespace tinyRPC::protobuf

#endif  
