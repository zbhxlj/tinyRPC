#include "StreamProtocol.h"

namespace tinyRPC {

FLARE_DEFINE_CLASS_DEPENDENCY_REGISTRY(client_side_stream_protocol_registry,
                                       StreamProtocol);
FLARE_DEFINE_CLASS_DEPENDENCY_REGISTRY(server_side_stream_protocol_registry,
                                       StreamProtocol);

}  // namespace tinyRPC
