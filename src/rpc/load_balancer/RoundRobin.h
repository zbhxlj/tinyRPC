#ifndef _SRC_RPC_LOAD_BALANCER_ROUND_ROBIN_H_
#define _SRC_RPC_LOAD_BALANCER_ROUND_ROBIN_H_

#include <atomic>
#include <memory>
#include <vector>

#include "../../base/Random.h"
#include "LoadBalancer.h"

namespace tinyRPC::load_balancer {

// A "real" load balancer. Only responsible for load balancing, has nothing
// to do about name resolving.
class RoundRobin : public LoadBalancer {
 public:
  ~RoundRobin();

  void SetPeers(std::vector<Endpoint> addresses) override;

  // `key` is ignored, as we select endpoints in a round-robin fashion.
  bool GetPeer(std::uint64_t key, Endpoint* addr, std::uintptr_t* ctx) override;

 private:
  struct Peers {
    std::vector<Endpoint> peers;
  };

  std::atomic<std::size_t> next_{Random()};  // FIXME: Make it thread-local.
  std::shared_ptr<Peers> endpoints_{std::make_shared<Peers>()};
};

}  // namespace tinyRPC::load_balancer

#endif  
