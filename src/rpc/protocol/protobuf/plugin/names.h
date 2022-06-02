#ifndef _SRC_RPC_PROTOCOL_PROTOBUF_PLUGIN_NAMES_H_
#define _SRC_RPC_PROTOCOL_PROTOBUF_PLUGIN_NAMES_H_

#include <string>

#include "google/protobuf/descriptor.h"

#include "code_writer.h"

namespace tinyRPC::protobuf::plugin {

// Several predefined insertion points.
constexpr auto kInsertionPointIncludes = "includes";
constexpr auto kInsertionPointNamespaceScope = "namespace_scope";
constexpr auto kInsertionPointGlobalScope = "global_scope";

// Convert Protocol Buffer's fully qualified name to cpp style.
std::string ToNativeName(const std::string& s);

// Get fully qualified type name of method's input type.
std::string GetInputType(const google::protobuf::MethodDescriptor* method);

// Get fully qualified type name of method's output type.
std::string GetOutputType(const google::protobuf::MethodDescriptor* method);

// Mangles service / stub's name.

// Basic service is exactly what `cc_generic_service = true` would generates.
std::string GetBasicServiceName(const google::protobuf::ServiceDescriptor* s);
std::string GetBasicStubName(const google::protobuf::ServiceDescriptor* s);

// Sync service is not inherently performs bad in flare since we're using fiber
// anyway.
//
// This stub also provides a better interface for calling streaming methods.
std::string GetSyncServiceName(const google::protobuf::ServiceDescriptor* s);
std::string GetSyncStubName(const google::protobuf::ServiceDescriptor* s);

}  // namespace tinyRPC::protobuf::plugin

#endif  
