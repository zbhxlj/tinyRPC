#ifndef _SRC_RPC_PROTOCOL_PROTOBUF_CALL_CONTEXT_H_
#define _SRC_RPC_PROTOCOL_PROTOBUF_CALL_CONTEXT_H_

#include <limits>

#include "google/protobuf/message.h"

#include "../../../base/MaybeOwning.h"
#include "../Controller.h"

namespace tinyRPC::protobuf {

// This call context is used when we're proactively making calls. That is, it's
// used at client side.
struct ProactiveCallContext : Controller {
  // Set if we're holding a response prototype (as opposed of a response
  // buffer.).
  bool expecting_stream;
  google::protobuf::Message* response_ptr = nullptr;
  const google::protobuf::Message* response_prototype = nullptr;

  // Method being called.
  const google::protobuf::MethodDescriptor* method;

  // Return `response_ptr` or create a new response buffer from
  // `response_prototype`, depending on whether `expecting_stream` is set.
  MaybeOwning<google::protobuf::Message> GetOrCreateResponse();

  ProactiveCallContext() {}
};

// This context is used when we're called passively, i.e., at server side.
struct PassiveCallContext : Controller {
  PassiveCallContext() {}

  // Not everyone set this. See implementation of the protocol object for
  // detail.
  const google::protobuf::MethodDescriptor* method = nullptr;

};

}  // namespace tinyRPC::protobuf

#endif  
