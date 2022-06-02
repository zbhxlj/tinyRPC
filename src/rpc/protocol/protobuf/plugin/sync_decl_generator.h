#ifndef _SRC_RPC_PROTOCOL_PROTOBUF_PLUGIN_SYNC_DECL_GENERATOR_H_
#define _SRC_RPC_PROTOCOL_PROTOBUF_PLUGIN_SYNC_DECL_GENERATOR_H_

#include "google/protobuf/descriptor.h"

#include "code_writer.h"

namespace tinyRPC::protobuf::plugin {

// This class generates synchronous version of classes.
class SyncDeclGenerator {
 public:
  void GenerateService(const google::protobuf::FileDescriptor* file,
                       const google::protobuf::ServiceDescriptor* service,
                       CodeWriter* writer);

  void GenerateStub(const google::protobuf::FileDescriptor* file,
                    const google::protobuf::ServiceDescriptor* service,
                    CodeWriter* writer);
};

}  // namespace tinyRPC::protobuf::plugin

#endif  
