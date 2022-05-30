#include "Composited.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace tinyRPC::message_dispatcher {

Composited::Composited(NameResolver* nr, std::unique_ptr<LoadBalancer> lb)
    : nr_(nr), lb_(std::move(lb)) {}

bool Composited::Open(const std::string& name) {
  nrv_ = nr_->StartResolving(name);
  if (!nrv_) {
    return false;
  }
  service_name_ = name;
  return true;
}

bool Composited::GetPeer(std::uint64_t key, Endpoint* addr,
                         std::uintptr_t* ctx) {
  auto version = nrv_->GetVersion();
  if (version != last_version_.load(std::memory_order_relaxed)) {
    std::scoped_lock _(reset_peers_lock_);
    if (version != last_version_.load(std::memory_order_relaxed)) {  // DCLP.
      std::vector<Endpoint> peers;
      nrv_->GetPeers(&peers);
      lb_->SetPeers(std::move(peers));
      last_version_.store(version, std::memory_order_relaxed);
    }
  }

  return lb_->GetPeer(key, addr, ctx);
}


}  
