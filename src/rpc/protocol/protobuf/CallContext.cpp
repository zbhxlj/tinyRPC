#include "CallContext.h"

namespace tinyRPC::protobuf {

MaybeOwning<google::protobuf::Message>
ProactiveCallContext::GetOrCreateResponse() {
  if (expecting_stream) {
    FLARE_CHECK(response_prototype);
    return MaybeOwning(owning, response_prototype->New());
  } else {
    auto rc = std::exchange(response_ptr, nullptr);
    FLARE_CHECK(rc);  // Otherwise it's a bug in `StreamCallGate`.
    return MaybeOwning(non_owning, rc);
  }
}

}  // namespace tinyRPC::protobuf
