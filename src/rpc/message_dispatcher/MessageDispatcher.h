#ifndef _SRC_RPC_MESSAGE_DISPATCHER_MESSAGE_DISPATCHER_H_
#define _SRC_RPC_MESSAGE_DISPATCHER_MESSAGE_DISPATCHER_H_

#include <chrono>
#include <string>

#include "../../base/DependencyRegistry.h"
#include "../../base/Endpoint.h"
#include "../load_balancer/LoadBalancer.h"

namespace tinyRPC {

// `MessageDispatcher` is responsible for choosing which server we should
// dispatch our RPC to.
//
// The `MessageDispatcher` may itself do naming resolution, or delegate it to
// some `NameResolver`.
//
// Load balancing / fault tolerance / naming resolution are all done here.
//
// Note that the implementation must be thread-safe.
class MessageDispatcher {
 public:
  virtual ~MessageDispatcher() = default;

  // Init with service name `name`.
  //
  // Format checking is done here.
  //
  // This service name is implied when `GetPeer` / `Feedback` is called.
  //
  virtual bool Open(const std::string& name) = 0;

  // `key` could be used for increase the cache hit rate of the downstream
  // services (if they implemented cache, of course).
  //
  // The dispatcher tries to dispatch requests with the same `key` to the
  // same group of servers.
  //
  // The implementation is required to avoid block at their best effort.
  virtual bool GetPeer(std::uint64_t key, Endpoint* addr,
                       std::uintptr_t* ctx) = 0;

  using Status = LoadBalancer::Status;

};

FLARE_DECLARE_CLASS_DEPENDENCY_REGISTRY(message_dispatcher_registry,
                                        MessageDispatcher);

}  // namespace tinyRPC

#define FLARE_RPC_REGISTER_MESSAGE_DISPATCHER(Name, Implementation)         \
  FLARE_REGISTER_CLASS_DEPENDENCY(flare::message_dispatcher_registry, Name, \
                                  Implementation);

#endif  
