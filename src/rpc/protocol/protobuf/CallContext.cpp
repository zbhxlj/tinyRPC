#include "CallContext.h"

namespace tinyRPC::protobuf {

MaybeOwning<google::protobuf::Message>
ProactiveCallContext::GetOrCreateResponse() {
  FLARE_CHECK(response_prototype);
  return MaybeOwning(owning, response_prototype->New());
}

}  // namespace tinyRPC::protobuf
