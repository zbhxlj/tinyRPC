#ifndef _SRC_RPC_LOAD_BALANCER_LOAD_BALANCER_H_
#define _SRC_RPC_LOAD_BALANCER_LOAD_BALANCER_H_

#include <chrono>
#include <cstdint>
#include <vector>

#include "../../base/DependencyRegistry.h"
#include "../../base/Endpoint.h"

namespace tinyRPC {

// `LoadBalancer` is responsible for selecting peer to send message.
//
// Each object is responsible for only one cluster of servers (that is,
// corresponding to one "name" resolved by `NameResolver`.).
//
// For example, we may have two `LoadBalancer`, one for selecting servers
// in "sunfish" cluster, while the other for "adx" cluster.
//
// Unless otherwise stated, THE IMPLEMENTATION MUST BE THREAD-SAFE.
class LoadBalancer {
 public:
  virtual ~LoadBalancer() = default;

  // Overwrites what we currently have.
  //
  // It's undefined to call this method concurrently (although this method can
  // be called concurrently with other methods such as `GetPeer()`) -- It simply
  // makes no sense in doing so.
  virtual void SetPeers(std::vector<Endpoint> addresses) = 0;

  virtual bool GetPeer(std::uint64_t key, Endpoint* addr,
                       std::uintptr_t* ctx) = 0;

  enum class Status {
    Success,
    Overloaded,  // Not implemented at this time.
    Failed
  };

};

FLARE_DECLARE_CLASS_DEPENDENCY_REGISTRY(load_balancer_registry, LoadBalancer);

}  // namespace tinyRPC

#define FLARE_RPC_REGISTER_LOAD_BALANCER(Name, Implementation)         \
  FLARE_REGISTER_CLASS_DEPENDENCY(tinyRPC::load_balancer_registry, Name, \
                                  Implementation);

#endif  
