#ifndef _SRC_TESTING_ENDPOINT_H_
#define _SRC_TESTING_ENDPOINT_H_

#include "../base/Endpoint.h"

namespace tinyRPC::testing {

// On Linux, binding on port zero automatically resolves to an ephemeral port.
// You might want to use this trick to chose your "available" port.

std::uint16_t PickAvailablePort(int type = SOCK_STREAM);

// Same as `PickAvailablePort`, using "127.0.0.1" as IP address.
Endpoint PickAvailableEndpoint(int type = SOCK_STREAM);

}  // namespace tinyRPC::testing

#endif  
