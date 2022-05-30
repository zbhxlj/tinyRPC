#ifndef _SRC_RPC_MESSAGE_DISPATCHER_COMPOSITED_H_
#define _SRC_RPC_MESSAGE_DISPATCHER_COMPOSITED_H_

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#include "../load_balancer/LoadBalancer.h"
#include "../message_dispatcher/MessageDispatcher.h"
#include "../name_resolver/NameResolver.h"

namespace tinyRPC::message_dispatcher {

// A composition of `NameResolver` and `LoadBalancer`.
class Composited : public MessageDispatcher {
 public:
  Composited(NameResolver* nr, std::unique_ptr<LoadBalancer> lb);

  bool Open(const std::string& name) override;
  bool GetPeer(std::uint64_t key, Endpoint* addr, std::uintptr_t* ctx) override;

 private:
  NameResolver* nr_;
  std::unique_ptr<NameResolutionView> nrv_;
  std::unique_ptr<LoadBalancer> lb_;
  std::string service_name_;

  std::mutex reset_peers_lock_;
  std::atomic<std::int64_t> last_version_ = NameResolutionView::kNewVersion;
};

}  // namespace tinyRPC::message_dispatcher

#endif  
