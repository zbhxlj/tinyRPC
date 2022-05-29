#ifndef _SRC_RPC_NAME_RESOLVER_LIST_H_
#define _SRC_RPC_NAME_RESOLVER_LIST_H_

#include <string>
#include <vector>

#include "NameResolver.h"
#include "NameResolverImpl.h"

namespace tinyRPC::name_resolver {

// name e.g.: 192.0.2.1:80,192.0.2.2:8080,[2001:db8::1]:8088,www.qq.com:443
//
// IP (v4 / v6) and domain.
class List : public NameResolverImpl {
 public:
  List();

 private:
  bool CheckValid(const std::string& name) override;

  bool GetRouteTable(const std::string& name, const std::string& old_signature,
                     std::vector<Endpoint>* new_address,
                     std::string* new_signature) override;
};

}  // namespace tinyRPC::name_resolver

#endif  
