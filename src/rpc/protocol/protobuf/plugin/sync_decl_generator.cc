#include "sync_decl_generator.h"

#include <string>
#include <vector>

#include "google/protobuf/compiler/cpp/cpp_helpers.h"

#include "../../../../base/String.h"
#include "names.h"

using namespace std::literals;

namespace tinyRPC::protobuf::plugin {

void SyncDeclGenerator::GenerateService(
    const google::protobuf::FileDescriptor* file,
    const google::protobuf::ServiceDescriptor* service, CodeWriter* writer) {
  // Generate service class' definition in header.
  std::vector<std::string> method_decls;
  for (int i = 0; i != service->method_count(); ++i) {
    auto&& method = service->method(i);
    std::string pattern;

    pattern =
        "virtual void {method}(\n"
        "    const {input_type}& request,\n"  // Ref here, not pointer.
        "    {output_type}* response,\n"
        "    ::flare::RpcServerController* controller);";
    method_decls.emplace_back() =
        Format(pattern, fmt::arg("method", method->name()),
               fmt::arg("input_type", GetInputType(method)),
               fmt::arg("output_type", GetOutputType(method)));
  }
  *writer->NewInsertionToHeader(kInsertionPointNamespaceScope) = Format(
      "class {service} : public ::google::protobuf::Service {{\n"
      " protected:\n"
      "  {service}() = default;\n"
      "\n"
      " public:\n"
      "  virtual ~{service}() = default;\n"
      "\n"
      "  {methods}\n"
      "\n"
      "  ///////////////////////////////////////////////\n"
      "  // Methods below are for internal use only.  //\n"
      "  ///////////////////////////////////////////////\n"
      "\n"
      "  const ::google::protobuf::ServiceDescriptor* GetDescriptor() final;\n"
      "\n"
      "  void CallMethod(const ::google::protobuf::MethodDescriptor* method,\n"
      "                  ::google::protobuf::RpcController* controller,\n"
      "                  const ::google::protobuf::Message* request,\n"
      "                  ::google::protobuf::Message* response,\n"
      "                  ::google::protobuf::Closure* done) override;\n"
      "\n"
      "  const ::google::protobuf::Message& GetRequestPrototype(\n"
      "      const ::google::protobuf::MethodDescriptor* method) const final;\n"
      "  const ::google::protobuf::Message& GetResponsePrototype(\n"
      "      const ::google::protobuf::MethodDescriptor* method) const final;\n"
      "\n"
      " private:\n"
      "  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS({service});\n"
      "}};\n"
      "\n",
      fmt::arg("stub", GetSyncStubName(service)),
      fmt::arg("service", GetSyncServiceName(service)),
      fmt::arg("methods", Replace(Join(method_decls, "\n"), "\n", "\n  ")));

  // Generate service's implementation.
  std::vector<std::string> call_method_impls;
  std::vector<std::string> get_request_prototype_impls;
  std::vector<std::string> get_response_prototype_impls;
  for (int i = 0; i != service->method_count(); ++i) {
    auto&& method = service->method(i);
    std::string pattern;

    pattern =
        "case {index}: {{\n"
        "  {method}(\n"
        "      *::google::protobuf::down_cast<const {input_type}*>(\n"
        "          request),\n"
        "      ::google::protobuf::down_cast<{output_type}*>(response),\n"
        "      ctlr);\n"
        "  done->Run();\n"
        "  break;\n"
        "}}";

    call_method_impls.emplace_back() =
        Format(pattern, fmt::arg("index", method->index()),
               fmt::arg("method", method->name()),
               fmt::arg("input_type", GetInputType(method)),
               fmt::arg("output_type", GetOutputType(method)));
    get_request_prototype_impls.emplace_back() = Format(
        "case {index}:\n"
        "  return {input_type}::default_instance();",
        fmt::arg("index", method->index()),
        fmt::arg("input_type", GetInputType(method)));
    get_response_prototype_impls.emplace_back() = Format(
        "case {index}:\n"
        "  return {output_type}::default_instance();",
        fmt::arg("index", method->index()),
        fmt::arg("output_type", GetOutputType(method)));
  }
  *writer->NewInsertionToSource(kInsertionPointNamespaceScope) += Format(
      "const ::google::protobuf::ServiceDescriptor*\n"
      "{service}::GetDescriptor() {{\n"
      "  return flare_rpc::GetServiceDescriptor({svc_idx});\n"
      "}}\n"
      "\n"
      "void {service}::CallMethod(\n"
      "    const ::google::protobuf::MethodDescriptor* method,\n"
      "    ::google::protobuf::RpcController* controller,\n"
      "    const ::google::protobuf::Message* request,\n"
      "    ::google::protobuf::Message* response,\n"
      "    ::google::protobuf::Closure* done) {{\n"
      "  GOOGLE_DCHECK_EQ(method->service(),\n"
      "                   flare_rpc::GetServiceDescriptor({svc_idx}));\n"
      "  auto ctlr = ::flare::down_cast<flare::RpcServerController>(\n"
      "      controller);\n"
      "  switch (method->index()) {{\n"
      "    {call_method_cases}\n"
      "  default:\n"
      "    GOOGLE_LOG(FATAL) <<\n"
      "        \"Bad method index; this should never happen.\";\n"
      "  }}\n"
      "}}\n"
      "\n"
      "const ::google::protobuf::Message& {service}::GetRequestPrototype(\n"
      "    const ::google::protobuf::MethodDescriptor* method) const {{\n"
      "  GOOGLE_DCHECK_EQ(method->service(),\n"
      "                   flare_rpc::GetServiceDescriptor({svc_idx}));\n"
      "  switch (method->index()) {{\n"
      "    {get_request_prototype_cases}\n"
      "  default:\n"
      "    GOOGLE_LOG(FATAL) <<\n"
      "        \"Bad method index; this should never happen.\";\n"
      "    return *::google::protobuf::MessageFactory::generated_factory()\n"
      "        ->GetPrototype(method->input_type());\n"
      "  }}\n"
      "}}\n"
      "\n"
      "const ::google::protobuf::Message& {service}::GetResponsePrototype(\n"
      "    const ::google::protobuf::MethodDescriptor* method) const {{\n"
      "  GOOGLE_DCHECK_EQ(method->service(),\n"
      "                   flare_rpc::GetServiceDescriptor({svc_idx}));\n"
      "  switch (method->index()) {{\n"
      "    {get_response_prototype_cases}\n"
      "  default:\n"
      "    GOOGLE_LOG(FATAL) <<\n"
      "        \"Bad method index; this should never happen.\";\n"
      "    return *::google::protobuf::MessageFactory::generated_factory()\n"
      "        ->GetPrototype(method->output_type());\n"
      "  }}\n"
      "}}\n"
      "\n",
      fmt::arg("file", file->name()),
      fmt::arg("service", GetSyncServiceName(service)),
      fmt::arg("file_ns", google::protobuf::compiler::cpp::FileLevelNamespace(
                              file->name())),
      fmt::arg("svc_idx", service->index()),
      fmt::arg("call_method_cases",
               Replace(Join(call_method_impls, "\n"), "\n", "\n    ")),
      fmt::arg(
          "get_request_prototype_cases",
          Replace(Join(get_request_prototype_impls, "\n"), "\n", "\n    ")),
      fmt::arg(
          "get_response_prototype_cases",
          Replace(Join(get_response_prototype_impls, "\n"), "\n", "\n    ")));

  for (int i = 0; i != service->method_count(); ++i) {
    auto&& method = service->method(i);
    std::string pattern;

    pattern =
        "void {service}::{method}(\n"
        "    const {input_type}& request,\n"  // Ref here, not pointer.
        "    {output_type}* response,\n"
        "    ::flare::RpcServerController* controller) {{\n"
        "  {body}\n"
        "}}";

    auto body = Format(
        "controller->SetFailed(\n"
        "    ::flare::rpc::STATUS_FAILED,\n"
        "    \"Method {method}() not implemented.\");",
        fmt::arg("method", method->name()));
    *writer->NewInsertionToSource(kInsertionPointNamespaceScope) =
        Format(pattern, fmt::arg("service", GetSyncServiceName(service)),
               fmt::arg("method", method->name()),
               fmt::arg("input_type", GetInputType(method)),
               fmt::arg("output_type", GetOutputType(method)),
               fmt::arg("body", body)) +
        "\n\n";
  }
}

void SyncDeclGenerator::GenerateStub(
    const google::protobuf::FileDescriptor* file,
    const google::protobuf::ServiceDescriptor* service, CodeWriter* writer) {
  // Code in header.
  std::vector<std::string> method_decls;
  for (int i = 0; i != service->method_count(); ++i) {
    auto&& method = service->method(i);
    std::string pattern;

    // TODO(luobogao): Support default controller (used when `controller` is not
    // specified or specified as `nullptr`.).
    // TBH I'm not sure if we should return `Expected<T>` or `optional<T>`.
    // The former looks more appropriate but our `Expected` has not been
    // thoroughly thought about.
    pattern =
        "::flare::Expected<{output_type},\n"
        "                  ::flare::Status>\n"
        "{method}(\n"
        "    const {input_type}& request,\n"
        "    ::flare::RpcClientController* controller);";

    method_decls.emplace_back() =
        Format(pattern, fmt::arg("method", method->name()),
               fmt::arg("input_type", GetInputType(method)),
               fmt::arg("output_type", GetOutputType(method)),
               fmt::arg("index", method->index()),
               fmt::arg("service", GetSyncServiceName(service)));
  }
  *writer->NewInsertionToHeader(kInsertionPointNamespaceScope) = Format(
      "class {stub} {{\n"
      "  using MaybeOwningChannel = ::flare::MaybeOwningArgument<\n"
      "      ::google::protobuf::RpcChannel>;\n"
      " public:\n"
      "  {stub}(MaybeOwningChannel channel)\n"
      "    : channel_(std::move(channel)) {{}}\n"
      "\n"
      "  {stub}(const std::string& uri);\n"
      "\n"
      "  {methods}\n"
      "\n"
      " private:\n"
      "  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS({stub});\n"
      "  ::flare::MaybeOwning<::google::protobuf::RpcChannel> channel_;\n"
      "}};\n"
      "\n",
      fmt::arg("stub", GetSyncStubName(service)),
      fmt::arg("methods", Replace(Join(method_decls, "\n"), "\n", "\n  ")));

  *writer->NewInsertionToSource(kInsertionPointNamespaceScope) = Format(
      "{stub}::{stub}(const std::string& uri) {{\n"
      "  channel_ = std::make_unique<flare::RpcChannel>(uri);\n"
      "}}"
      "\n",
      fmt::arg("stub", GetSyncStubName(service)));

  for (int i = 0; i != service->method_count(); ++i) {
    auto&& method = service->method(i);
    std::string pattern;

    // TODO(luobogao): Support default controller (used when `controller` is not
    // specified or specified as `nullptr`.).
    pattern =
        "::flare::Expected<{output_type}, ::flare::Status>\n"
        "{stub}::{method}(\n"
        "    const {input_type}& request,\n"
        "    ::flare::RpcClientController* ctlr) {{\n"
        "  {output_type} rc;\n"
        "  channel_->CallMethod(\n"
        "      flare_rpc::GetServiceDescriptor({svc_idx})->method({index}),\n"
        "      ctlr, &request, &rc, nullptr);\n"
        "  if (!ctlr->Failed()) {{\n"
        "    return rc;\n"
        "  }}\n"
        // By reaching here, the call must have failed.
        "  return flare::Status(ctlr->ErrorCode(), ctlr->ErrorText());\n"
        "}}";

    *writer->NewInsertionToSource(kInsertionPointNamespaceScope) =
        Format(pattern, fmt::arg("stub", GetSyncStubName(service)),
               fmt::arg("method", method->name()),
               fmt::arg("input_type", GetInputType(method)),
               fmt::arg("output_type", GetOutputType(method)),
               fmt::arg("svc_idx", service->index()),
               fmt::arg("index", method->index()),
               fmt::arg("service", GetSyncServiceName(service))) +
        "\n\n";
  }
}

}  // namespace tinyRPC::protobuf::plugin
