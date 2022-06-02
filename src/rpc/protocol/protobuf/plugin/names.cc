#include "names.h"

#include <string>

#include "../../../../base/String.h"

namespace tinyRPC::protobuf::plugin {

std::string ToNativeName(const std::string& s) { return Replace(s, ".", "::"); }

std::string GetInputType(const google::protobuf::MethodDescriptor* method) {
  return "::" + ToNativeName(method->input_type()->full_name());
}

std::string GetOutputType(const google::protobuf::MethodDescriptor* method) {
  return "::" + ToNativeName(method->output_type()->full_name());
}

std::string GetBasicServiceName(const google::protobuf::ServiceDescriptor* s) {
  return "Basic" + s->name();
}

std::string GetBasicStubName(const google::protobuf::ServiceDescriptor* s) {
  return s->name() + "_BasicStub";
}

std::string GetSyncServiceName(const google::protobuf::ServiceDescriptor* s) {
  return "Sync" + s->name();
}

std::string GetSyncStubName(const google::protobuf::ServiceDescriptor* s) {
  return s->name() + "_SyncStub";
}

}  // namespace tinyRPC::protobuf::plugin
