#ifndef _SRC_RPC_NAME_RESOLVER_NAME_RESOLVER_H_
#define _SRC_RPC_NAME_RESOLVER_NAME_RESOLVER_H_

#include <memory>
#include <string>
#include <vector>

#include "../../base/DependencyRegistry.h"
#include "../../base/Endpoint.h"

namespace tinyRPC {

// `NameResolutionView` is responsible for getting latest name resolution
// version and peer list.
class NameResolutionView {
 public:
  virtual ~NameResolutionView() = default;

  // These constants are intended for `GetVersion` to return if it sees
  // appropriate.
  //
  // `kNewVersion`, if returned, is always treated as a new "version", even if
  // the previous call also returned the same value (i.e., `kNewVersion`). This
  // can be handy for `NameResolutionView`s that do not implement cache
  // behavior.
  //
  // `kUseGenericCache`, if returned, indicates the implementation does not
  // implement cache behavior, but caching is allowed, so the framework should
  // do caching itself. For now this is done by calling `Resolve()` in dedicated
  // thread periodically.
  static constexpr std::int64_t kNewVersion = -1;
  static constexpr std::int64_t kUseGenericCache = -2;

  // Returns "version" of the resolving result. Each time there's a
  // change, the value returned by this method should be increased.
  //
  // This may be used by load balancers for detecting changes in resolution and
  // take actions accordingly.
  //
  // In deed there's a chance that the result changed between calling
  // `GetVersion` and `Resolve`. But I don't see why this will hurt.
  //
  // @sa: Special values are defined above.
  virtual std::int64_t GetVersion() = 0;

  // We encourage the implementation to avoid blocking at their best effort.
  // However, since we also implemented our own "generic" cache, it's allowed
  // for the implementation not to implement cache behavior at all.
  virtual void GetPeers(std::vector<Endpoint>* addresses) = 0;
};

// `NameResolver` is responsible for resolving name to a list of
// server addresses.
//
// The implementation may cache the results as it sees appropriate.
//
// Objects of this class is likely to be used as a singleton.
class NameResolver {
 public:
  virtual ~NameResolver() = default;

  // Returns `nullptr` if `name` is evidently not resolvable.
  virtual std::unique_ptr<NameResolutionView> StartResolving(
      const std::string& name) = 0;
};

// Name resolver are different in that we do not want a collection of
// instances of them, but a global singleton.
FLARE_DECLARE_OBJECT_DEPENDENCY_REGISTRY(name_resolver_registry, NameResolver);

}  // namespace tinyRPC

#define FLARE_RPC_REGISTER_NAME_RESOLVER(Name, Implementation)               \
  FLARE_REGISTER_OBJECT_DEPENDENCY(tinyRPC::name_resolver_registry, Name, [] { \
    return std::make_unique<Implementation>();                               \
  })

#endif  
