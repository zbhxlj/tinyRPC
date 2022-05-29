#ifndef _SRC_RPC_NAME_RESOLVER_UTIL_DOMAIN_NAME_RESOLVER_H_
#define _SRC_RPC_NAME_RESOLVER_UTIL_DOMAIN_NAME_RESOLVER_H_

#include <string>
#include <vector>

#include "../../../base/Endpoint.h"

namespace tinyRPC::name_resolver::util {

/// simple internet domain resolver
bool ResolveDomain(const std::string& domain, std::uint16_t port,
                   std::vector<Endpoint>* addresses);

}  // namespace tinyRPC::name_resolver::util

#endif  
