// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: echo_service.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "echo_service.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace tinyRPC {
namespace testing {
class EchoRequestDefaultTypeInternal {
public:
 ::google::protobuf::internal::ExplicitlyConstructed<EchoRequest>
     _instance;
} _EchoRequest_default_instance_;
class EchoResponseDefaultTypeInternal {
public:
 ::google::protobuf::internal::ExplicitlyConstructed<EchoResponse>
     _instance;
} _EchoResponse_default_instance_;

namespace protobuf_echo_5fservice_2eproto {


namespace {

::google::protobuf::Metadata file_level_metadata[2];
const ::google::protobuf::ServiceDescriptor* file_level_service_descriptors[1];

}  // namespace

PROTOBUF_CONSTEXPR_VAR ::google::protobuf::internal::ParseTableField
    const TableStruct::entries[] GOOGLE_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
  {0, 0, 0, ::google::protobuf::internal::kInvalidMask, 0, 0},
};

PROTOBUF_CONSTEXPR_VAR ::google::protobuf::internal::AuxillaryParseTableField
    const TableStruct::aux[] GOOGLE_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
  ::google::protobuf::internal::AuxillaryParseTableField(),
};
PROTOBUF_CONSTEXPR_VAR ::google::protobuf::internal::ParseTable const
    TableStruct::schema[] GOOGLE_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
  { NULL, NULL, 0, -1, -1, -1, -1, NULL, false },
  { NULL, NULL, 0, -1, -1, -1, -1, NULL, false },
};

const ::google::protobuf::uint32 TableStruct::offsets[] GOOGLE_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(EchoRequest, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(EchoRequest, body_),
  ~0u,  // no _has_bits_
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(EchoResponse, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(EchoResponse, body_),
};
static const ::google::protobuf::internal::MigrationSchema schemas[] GOOGLE_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, sizeof(EchoRequest)},
  { 6, -1, sizeof(EchoResponse)},
};

static ::google::protobuf::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::google::protobuf::Message*>(&_EchoRequest_default_instance_),
  reinterpret_cast<const ::google::protobuf::Message*>(&_EchoResponse_default_instance_),
};

namespace {

void protobuf_AssignDescriptors() {
  AddDescriptors();
  ::google::protobuf::MessageFactory* factory = NULL;
  AssignDescriptors(
      "echo_service.proto", schemas, file_default_instances, TableStruct::offsets, factory,
      file_level_metadata, NULL, file_level_service_descriptors);
}

void protobuf_AssignDescriptorsOnce() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &protobuf_AssignDescriptors);
}

void protobuf_RegisterTypes(const ::std::string&) GOOGLE_ATTRIBUTE_COLD;
void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::internal::RegisterAllTypes(file_level_metadata, 2);
}

}  // namespace
void TableStruct::InitDefaultsImpl() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::internal::InitProtobufDefaults();
  _EchoRequest_default_instance_._instance.DefaultConstruct();
  ::google::protobuf::internal::OnShutdownDestroyMessage(
      &_EchoRequest_default_instance_);_EchoResponse_default_instance_._instance.DefaultConstruct();
  ::google::protobuf::internal::OnShutdownDestroyMessage(
      &_EchoResponse_default_instance_);}

void InitDefaults() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &TableStruct::InitDefaultsImpl);
}
namespace {
void AddDescriptorsImpl() {
  InitDefaults();
  static const char descriptor[] GOOGLE_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
      "\n\022echo_service.proto\022\017tinyRPC.testing\"\033\n"
      "\013EchoRequest\022\014\n\004body\030\001 \001(\t\"\034\n\014EchoRespon"
      "se\022\014\n\004body\030\002 \001(\t2\261\001\n\013EchoService\022E\n\004Echo"
      "\022\034.tinyRPC.testing.EchoRequest\032\035.tinyRPC"
      ".testing.EchoResponse\"\000\022[\n\032EchoWithMaxOn"
      "goingRequests\022\034.tinyRPC.testing.EchoRequ"
      "est\032\035.tinyRPC.testing.EchoResponse\"\000B\003\200\001"
      "\001b\006proto3"
  };
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
      descriptor, 289);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "echo_service.proto", &protobuf_RegisterTypes);
}
} // anonymous namespace

void AddDescriptors() {
  static GOOGLE_PROTOBUF_DECLARE_ONCE(once);
  ::google::protobuf::GoogleOnceInit(&once, &AddDescriptorsImpl);
}
// Force AddDescriptors() to be called at dynamic initialization time.
struct StaticDescriptorInitializer {
  StaticDescriptorInitializer() {
    AddDescriptors();
  }
} static_descriptor_initializer;

}  // namespace protobuf_echo_5fservice_2eproto


// ===================================================================

#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int EchoRequest::kBodyFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

EchoRequest::EchoRequest()
  : ::google::protobuf::Message(), _internal_metadata_(NULL) {
  if (GOOGLE_PREDICT_TRUE(this != internal_default_instance())) {
    protobuf_echo_5fservice_2eproto::InitDefaults();
  }
  SharedCtor();
  // @@protoc_insertion_point(constructor:tinyRPC.testing.EchoRequest)
}
EchoRequest::EchoRequest(const EchoRequest& from)
  : ::google::protobuf::Message(),
      _internal_metadata_(NULL),
      _cached_size_(0) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  body_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.body().size() > 0) {
    body_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.body_);
  }
  // @@protoc_insertion_point(copy_constructor:tinyRPC.testing.EchoRequest)
}

void EchoRequest::SharedCtor() {
  body_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  _cached_size_ = 0;
}

EchoRequest::~EchoRequest() {
  // @@protoc_insertion_point(destructor:tinyRPC.testing.EchoRequest)
  SharedDtor();
}

void EchoRequest::SharedDtor() {
  body_.DestroyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}

void EchoRequest::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* EchoRequest::descriptor() {
  protobuf_echo_5fservice_2eproto::protobuf_AssignDescriptorsOnce();
  return protobuf_echo_5fservice_2eproto::file_level_metadata[kIndexInFileMessages].descriptor;
}

const EchoRequest& EchoRequest::default_instance() {
  protobuf_echo_5fservice_2eproto::InitDefaults();
  return *internal_default_instance();
}

EchoRequest* EchoRequest::New(::google::protobuf::Arena* arena) const {
  EchoRequest* n = new EchoRequest;
  if (arena != NULL) {
    arena->Own(n);
  }
  return n;
}

void EchoRequest::Clear() {
// @@protoc_insertion_point(message_clear_start:tinyRPC.testing.EchoRequest)
  ::google::protobuf::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  body_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  _internal_metadata_.Clear();
}

bool EchoRequest::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:tinyRPC.testing.EchoRequest)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(127u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // string body = 1;
      case 1: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(10u /* 10 & 0xFF */)) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_body()));
          DO_(::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
            this->body().data(), static_cast<int>(this->body().length()),
            ::google::protobuf::internal::WireFormatLite::PARSE,
            "tinyRPC.testing.EchoRequest.body"));
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, _internal_metadata_.mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:tinyRPC.testing.EchoRequest)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:tinyRPC.testing.EchoRequest)
  return false;
#undef DO_
}

void EchoRequest::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:tinyRPC.testing.EchoRequest)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // string body = 1;
  if (this->body().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->body().data(), static_cast<int>(this->body().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "tinyRPC.testing.EchoRequest.body");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      1, this->body(), output);
  }

  if ((_internal_metadata_.have_unknown_fields() &&  ::google::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        (::google::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()), output);
  }
  // @@protoc_insertion_point(serialize_end:tinyRPC.testing.EchoRequest)
}

::google::protobuf::uint8* EchoRequest::InternalSerializeWithCachedSizesToArray(
    bool deterministic, ::google::protobuf::uint8* target) const {
  (void)deterministic; // Unused
  // @@protoc_insertion_point(serialize_to_array_start:tinyRPC.testing.EchoRequest)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // string body = 1;
  if (this->body().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->body().data(), static_cast<int>(this->body().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "tinyRPC.testing.EchoRequest.body");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->body(), target);
  }

  if ((_internal_metadata_.have_unknown_fields() &&  ::google::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        (::google::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:tinyRPC.testing.EchoRequest)
  return target;
}

size_t EchoRequest::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:tinyRPC.testing.EchoRequest)
  size_t total_size = 0;

  if ((_internal_metadata_.have_unknown_fields() &&  ::google::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        (::google::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()));
  }
  // string body = 1;
  if (this->body().size() > 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::StringSize(
        this->body());
  }

  int cached_size = ::google::protobuf::internal::ToCachedSize(total_size);
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = cached_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void EchoRequest::MergeFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:tinyRPC.testing.EchoRequest)
  GOOGLE_DCHECK_NE(&from, this);
  const EchoRequest* source =
      ::google::protobuf::internal::DynamicCastToGenerated<const EchoRequest>(
          &from);
  if (source == NULL) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:tinyRPC.testing.EchoRequest)
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:tinyRPC.testing.EchoRequest)
    MergeFrom(*source);
  }
}

void EchoRequest::MergeFrom(const EchoRequest& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:tinyRPC.testing.EchoRequest)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.body().size() > 0) {

    body_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.body_);
  }
}

void EchoRequest::CopyFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:tinyRPC.testing.EchoRequest)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void EchoRequest::CopyFrom(const EchoRequest& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:tinyRPC.testing.EchoRequest)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool EchoRequest::IsInitialized() const {
  return true;
}

void EchoRequest::Swap(EchoRequest* other) {
  if (other == this) return;
  InternalSwap(other);
}
void EchoRequest::InternalSwap(EchoRequest* other) {
  using std::swap;
  body_.Swap(&other->body_);
  _internal_metadata_.Swap(&other->_internal_metadata_);
  swap(_cached_size_, other->_cached_size_);
}

::google::protobuf::Metadata EchoRequest::GetMetadata() const {
  protobuf_echo_5fservice_2eproto::protobuf_AssignDescriptorsOnce();
  return protobuf_echo_5fservice_2eproto::file_level_metadata[kIndexInFileMessages];
}

#if PROTOBUF_INLINE_NOT_IN_HEADERS
// EchoRequest

// string body = 1;
void EchoRequest::clear_body() {
  body_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
const ::std::string& EchoRequest::body() const {
  // @@protoc_insertion_point(field_get:tinyRPC.testing.EchoRequest.body)
  return body_.GetNoArena();
}
void EchoRequest::set_body(const ::std::string& value) {
  
  body_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:tinyRPC.testing.EchoRequest.body)
}
#if LANG_CXX11
void EchoRequest::set_body(::std::string&& value) {
  
  body_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:tinyRPC.testing.EchoRequest.body)
}
#endif
void EchoRequest::set_body(const char* value) {
  GOOGLE_DCHECK(value != NULL);
  
  body_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:tinyRPC.testing.EchoRequest.body)
}
void EchoRequest::set_body(const char* value, size_t size) {
  
  body_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:tinyRPC.testing.EchoRequest.body)
}
::std::string* EchoRequest::mutable_body() {
  
  // @@protoc_insertion_point(field_mutable:tinyRPC.testing.EchoRequest.body)
  return body_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
::std::string* EchoRequest::release_body() {
  // @@protoc_insertion_point(field_release:tinyRPC.testing.EchoRequest.body)
  
  return body_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
void EchoRequest::set_allocated_body(::std::string* body) {
  if (body != NULL) {
    
  } else {
    
  }
  body_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), body);
  // @@protoc_insertion_point(field_set_allocated:tinyRPC.testing.EchoRequest.body)
}

#endif  // PROTOBUF_INLINE_NOT_IN_HEADERS

// ===================================================================

#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int EchoResponse::kBodyFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

EchoResponse::EchoResponse()
  : ::google::protobuf::Message(), _internal_metadata_(NULL) {
  if (GOOGLE_PREDICT_TRUE(this != internal_default_instance())) {
    protobuf_echo_5fservice_2eproto::InitDefaults();
  }
  SharedCtor();
  // @@protoc_insertion_point(constructor:tinyRPC.testing.EchoResponse)
}
EchoResponse::EchoResponse(const EchoResponse& from)
  : ::google::protobuf::Message(),
      _internal_metadata_(NULL),
      _cached_size_(0) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  body_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  if (from.body().size() > 0) {
    body_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.body_);
  }
  // @@protoc_insertion_point(copy_constructor:tinyRPC.testing.EchoResponse)
}

void EchoResponse::SharedCtor() {
  body_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  _cached_size_ = 0;
}

EchoResponse::~EchoResponse() {
  // @@protoc_insertion_point(destructor:tinyRPC.testing.EchoResponse)
  SharedDtor();
}

void EchoResponse::SharedDtor() {
  body_.DestroyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}

void EchoResponse::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* EchoResponse::descriptor() {
  protobuf_echo_5fservice_2eproto::protobuf_AssignDescriptorsOnce();
  return protobuf_echo_5fservice_2eproto::file_level_metadata[kIndexInFileMessages].descriptor;
}

const EchoResponse& EchoResponse::default_instance() {
  protobuf_echo_5fservice_2eproto::InitDefaults();
  return *internal_default_instance();
}

EchoResponse* EchoResponse::New(::google::protobuf::Arena* arena) const {
  EchoResponse* n = new EchoResponse;
  if (arena != NULL) {
    arena->Own(n);
  }
  return n;
}

void EchoResponse::Clear() {
// @@protoc_insertion_point(message_clear_start:tinyRPC.testing.EchoResponse)
  ::google::protobuf::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  body_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  _internal_metadata_.Clear();
}

bool EchoResponse::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:tinyRPC.testing.EchoResponse)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoffNoLastTag(127u);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // string body = 2;
      case 2: {
        if (static_cast< ::google::protobuf::uint8>(tag) ==
            static_cast< ::google::protobuf::uint8>(18u /* 18 & 0xFF */)) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_body()));
          DO_(::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
            this->body().data(), static_cast<int>(this->body().length()),
            ::google::protobuf::internal::WireFormatLite::PARSE,
            "tinyRPC.testing.EchoResponse.body"));
        } else {
          goto handle_unusual;
        }
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, _internal_metadata_.mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:tinyRPC.testing.EchoResponse)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:tinyRPC.testing.EchoResponse)
  return false;
#undef DO_
}

void EchoResponse::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:tinyRPC.testing.EchoResponse)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // string body = 2;
  if (this->body().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->body().data(), static_cast<int>(this->body().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "tinyRPC.testing.EchoResponse.body");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      2, this->body(), output);
  }

  if ((_internal_metadata_.have_unknown_fields() &&  ::google::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        (::google::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()), output);
  }
  // @@protoc_insertion_point(serialize_end:tinyRPC.testing.EchoResponse)
}

::google::protobuf::uint8* EchoResponse::InternalSerializeWithCachedSizesToArray(
    bool deterministic, ::google::protobuf::uint8* target) const {
  (void)deterministic; // Unused
  // @@protoc_insertion_point(serialize_to_array_start:tinyRPC.testing.EchoResponse)
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // string body = 2;
  if (this->body().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->body().data(), static_cast<int>(this->body().length()),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "tinyRPC.testing.EchoResponse.body");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        2, this->body(), target);
  }

  if ((_internal_metadata_.have_unknown_fields() &&  ::google::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        (::google::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:tinyRPC.testing.EchoResponse)
  return target;
}

size_t EchoResponse::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:tinyRPC.testing.EchoResponse)
  size_t total_size = 0;

  if ((_internal_metadata_.have_unknown_fields() &&  ::google::protobuf::internal::GetProto3PreserveUnknownsDefault())) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        (::google::protobuf::internal::GetProto3PreserveUnknownsDefault()   ? _internal_metadata_.unknown_fields()   : _internal_metadata_.default_instance()));
  }
  // string body = 2;
  if (this->body().size() > 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::StringSize(
        this->body());
  }

  int cached_size = ::google::protobuf::internal::ToCachedSize(total_size);
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = cached_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void EchoResponse::MergeFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:tinyRPC.testing.EchoResponse)
  GOOGLE_DCHECK_NE(&from, this);
  const EchoResponse* source =
      ::google::protobuf::internal::DynamicCastToGenerated<const EchoResponse>(
          &from);
  if (source == NULL) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:tinyRPC.testing.EchoResponse)
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:tinyRPC.testing.EchoResponse)
    MergeFrom(*source);
  }
}

void EchoResponse::MergeFrom(const EchoResponse& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:tinyRPC.testing.EchoResponse)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::google::protobuf::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.body().size() > 0) {

    body_.AssignWithDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), from.body_);
  }
}

void EchoResponse::CopyFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:tinyRPC.testing.EchoResponse)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void EchoResponse::CopyFrom(const EchoResponse& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:tinyRPC.testing.EchoResponse)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool EchoResponse::IsInitialized() const {
  return true;
}

void EchoResponse::Swap(EchoResponse* other) {
  if (other == this) return;
  InternalSwap(other);
}
void EchoResponse::InternalSwap(EchoResponse* other) {
  using std::swap;
  body_.Swap(&other->body_);
  _internal_metadata_.Swap(&other->_internal_metadata_);
  swap(_cached_size_, other->_cached_size_);
}

::google::protobuf::Metadata EchoResponse::GetMetadata() const {
  protobuf_echo_5fservice_2eproto::protobuf_AssignDescriptorsOnce();
  return protobuf_echo_5fservice_2eproto::file_level_metadata[kIndexInFileMessages];
}

#if PROTOBUF_INLINE_NOT_IN_HEADERS
// EchoResponse

// string body = 2;
void EchoResponse::clear_body() {
  body_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
const ::std::string& EchoResponse::body() const {
  // @@protoc_insertion_point(field_get:tinyRPC.testing.EchoResponse.body)
  return body_.GetNoArena();
}
void EchoResponse::set_body(const ::std::string& value) {
  
  body_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:tinyRPC.testing.EchoResponse.body)
}
#if LANG_CXX11
void EchoResponse::set_body(::std::string&& value) {
  
  body_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:tinyRPC.testing.EchoResponse.body)
}
#endif
void EchoResponse::set_body(const char* value) {
  GOOGLE_DCHECK(value != NULL);
  
  body_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:tinyRPC.testing.EchoResponse.body)
}
void EchoResponse::set_body(const char* value, size_t size) {
  
  body_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:tinyRPC.testing.EchoResponse.body)
}
::std::string* EchoResponse::mutable_body() {
  
  // @@protoc_insertion_point(field_mutable:tinyRPC.testing.EchoResponse.body)
  return body_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
::std::string* EchoResponse::release_body() {
  // @@protoc_insertion_point(field_release:tinyRPC.testing.EchoResponse.body)
  
  return body_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
void EchoResponse::set_allocated_body(::std::string* body) {
  if (body != NULL) {
    
  } else {
    
  }
  body_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), body);
  // @@protoc_insertion_point(field_set_allocated:tinyRPC.testing.EchoResponse.body)
}

#endif  // PROTOBUF_INLINE_NOT_IN_HEADERS

// ===================================================================

EchoService::~EchoService() {}

const ::google::protobuf::ServiceDescriptor* EchoService::descriptor() {
  protobuf_echo_5fservice_2eproto::protobuf_AssignDescriptorsOnce();
  return protobuf_echo_5fservice_2eproto::file_level_service_descriptors[0];
}

const ::google::protobuf::ServiceDescriptor* EchoService::GetDescriptor() {
  return descriptor();
}

void EchoService::Echo(::google::protobuf::RpcController* controller,
                         const ::tinyRPC::testing::EchoRequest*,
                         ::tinyRPC::testing::EchoResponse*,
                         ::google::protobuf::Closure* done) {
  controller->SetFailed("Method Echo() not implemented.");
  done->Run();
}

void EchoService::EchoWithMaxOngoingRequests(::google::protobuf::RpcController* controller,
                         const ::tinyRPC::testing::EchoRequest*,
                         ::tinyRPC::testing::EchoResponse*,
                         ::google::protobuf::Closure* done) {
  controller->SetFailed("Method EchoWithMaxOngoingRequests() not implemented.");
  done->Run();
}

void EchoService::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                             ::google::protobuf::RpcController* controller,
                             const ::google::protobuf::Message* request,
                             ::google::protobuf::Message* response,
                             ::google::protobuf::Closure* done) {
  GOOGLE_DCHECK_EQ(method->service(), protobuf_echo_5fservice_2eproto::file_level_service_descriptors[0]);
  switch(method->index()) {
    case 0:
      Echo(controller,
             ::google::protobuf::down_cast<const ::tinyRPC::testing::EchoRequest*>(request),
             ::google::protobuf::down_cast< ::tinyRPC::testing::EchoResponse*>(response),
             done);
      break;
    case 1:
      EchoWithMaxOngoingRequests(controller,
             ::google::protobuf::down_cast<const ::tinyRPC::testing::EchoRequest*>(request),
             ::google::protobuf::down_cast< ::tinyRPC::testing::EchoResponse*>(response),
             done);
      break;
    default:
      GOOGLE_LOG(FATAL) << "Bad method index; this should never happen.";
      break;
  }
}

const ::google::protobuf::Message& EchoService::GetRequestPrototype(
    const ::google::protobuf::MethodDescriptor* method) const {
  GOOGLE_DCHECK_EQ(method->service(), descriptor());
  switch(method->index()) {
    case 0:
      return ::tinyRPC::testing::EchoRequest::default_instance();
    case 1:
      return ::tinyRPC::testing::EchoRequest::default_instance();
    default:
      GOOGLE_LOG(FATAL) << "Bad method index; this should never happen.";
      return *::google::protobuf::MessageFactory::generated_factory()
          ->GetPrototype(method->input_type());
  }
}

const ::google::protobuf::Message& EchoService::GetResponsePrototype(
    const ::google::protobuf::MethodDescriptor* method) const {
  GOOGLE_DCHECK_EQ(method->service(), descriptor());
  switch(method->index()) {
    case 0:
      return ::tinyRPC::testing::EchoResponse::default_instance();
    case 1:
      return ::tinyRPC::testing::EchoResponse::default_instance();
    default:
      GOOGLE_LOG(FATAL) << "Bad method index; this should never happen.";
      return *::google::protobuf::MessageFactory::generated_factory()
          ->GetPrototype(method->output_type());
  }
}

EchoService_Stub::EchoService_Stub(::google::protobuf::RpcChannel* channel)
  : channel_(channel), owns_channel_(false) {}
EchoService_Stub::EchoService_Stub(
    ::google::protobuf::RpcChannel* channel,
    ::google::protobuf::Service::ChannelOwnership ownership)
  : channel_(channel),
    owns_channel_(ownership == ::google::protobuf::Service::STUB_OWNS_CHANNEL) {}
EchoService_Stub::~EchoService_Stub() {
  if (owns_channel_) delete channel_;
}

void EchoService_Stub::Echo(::google::protobuf::RpcController* controller,
                              const ::tinyRPC::testing::EchoRequest* request,
                              ::tinyRPC::testing::EchoResponse* response,
                              ::google::protobuf::Closure* done) {
  channel_->CallMethod(descriptor()->method(0),
                       controller, request, response, done);
}
void EchoService_Stub::EchoWithMaxOngoingRequests(::google::protobuf::RpcController* controller,
                              const ::tinyRPC::testing::EchoRequest* request,
                              ::tinyRPC::testing::EchoResponse* response,
                              ::google::protobuf::Closure* done) {
  channel_->CallMethod(descriptor()->method(1),
                       controller, request, response, done);
}

// @@protoc_insertion_point(namespace_scope)

}  // namespace testing
}  // namespace tinyRPC

// @@protoc_insertion_point(global_scope)