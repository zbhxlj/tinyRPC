#include "RoundRobin.h"

#include <memory>
#include <utility>

namespace tinyRPC::load_balancer {

FLARE_RPC_REGISTER_LOAD_BALANCER("rr", RoundRobin);

RoundRobin::~RoundRobin() {}

void RoundRobin::SetPeers(std::vector<Endpoint> addresses) {
  auto new_peers = std::make_shared<Peers>();
  new_peers->peers = std::move(addresses);
  endpoints_.reset(new_peers);
}

bool RoundRobin::GetPeer(std::uint64_t key, Endpoint* addr,
                         std::uintptr_t* ctx) {
  if (FLARE_UNLIKELY(endpoints_->peers.empty())) {
    return false;
  }
  *addr = endpoints_->peers[next_.fetch_add(1) % endpoints_->peers.size()];
  return true;
}

}  // namespace tinyRPC::load_balancer
