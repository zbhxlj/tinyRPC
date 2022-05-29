#ifndef _SRC_RPC_NAME_RESOLVER_NAME_RESOLVER_IMPL_H_
#define _SRC_RPC_NAME_RESOLVER_NAME_RESOLVER_IMPL_H_

#include <optional>
#include <shared_mutex>

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../base/Endpoint.h"
#include "NameResolver.h"
#include "NameResolverUpdater.h"

namespace tinyRPC::name_resolver {

// `NameResolverImpl` is the basic implementation of NameResolver.
// It can be used to update and cache route results.
class NameResolverImpl : public NameResolver {
 public:
  struct RouteInfo {
    RouteInfo() = default;
    // Sorted by string of endpoint.
    std::vector<Endpoint> route_table;
    std::atomic<int64_t> version = 0;
    std::shared_mutex route_mutex;
  };
  virtual ~NameResolverImpl();
  std::unique_ptr<NameResolutionView> StartResolving(
      const std::string& name) override;

 protected:
  // Subclass wanting to update their route info periodically should call
  // this method to initialize `updater_`.
  NameResolverUpdater* GetUpdater();
  // If name does not exist in the route, we will insert.
  // Returns the pointer of routeinfo and if it's the first time.
  std::pair<std::shared_ptr<RouteInfo>, bool> GetRouteInfo(
      const std::string& name);
  // UpdateRouteTable
  void UpdateRoute(const std::string& name,
                   std::shared_ptr<RouteInfo> route_info_ptr);
  // Sub-class can do custom pre-check if needed.
  virtual bool CheckValid(const std::string& name) { return true; }
  // Signature is the optional field that may be used by child class.
  // If new_signature is set with not empty value and equal to the
  // old_signature. We will consider the route address has not changed and
  // directly return. We will set the value of old_signature to new_signature
  // for the next turn.
  virtual bool GetRouteTable(const std::string& name,
                             const std::string& old_signature,
                             std::vector<Endpoint>* new_address,
                             std::string* new_signature) = 0;

 protected:
  std::shared_mutex name_mutex_;
  std::unordered_map<std::string, std::shared_ptr<RouteInfo>> name_route_;
  NameResolverUpdater* updater_;
  std::map<std::string, std::string> name_signatures_;
};

// Basic implementation of NameResolutionView corresponding NameResolverImpl.
class NameResolutionViewImpl : public NameResolutionView {
 public:
  explicit NameResolutionViewImpl(
      std::shared_ptr<NameResolverImpl::RouteInfo> route);
  virtual ~NameResolutionViewImpl();
  std::int64_t GetVersion() override;
  void GetPeers(std::vector<Endpoint>* addresses) override;

 private:
  std::shared_ptr<NameResolverImpl::RouteInfo> route_;
};

}  // namespace tinyRPC::name_resolver

#endif  
