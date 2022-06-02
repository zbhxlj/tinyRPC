#include "ServiceMethodLocator.h"

#include <set>

#include "gtest/gtest.h"

#include "../../../testing/main.h"
#include "../../../testing/echo_service.pb.h"

namespace tinyRPC::protobuf {

inline constexpr struct FancyProtocol {
  using MethodKey = const void*;  // Pointer to method descriptor.
} fancy;

struct Dummy : public testing::EchoService {
} dummy;

auto service = dummy.GetDescriptor();

std::set<std::string> server_methods;

void ServerAddCallback(const google::protobuf::MethodDescriptor* method) {
  server_methods.insert(method->full_name());
  ServiceMethodLocator::Instance()->RegisterMethod(fancy, method, method);
}

void ServerRemoveCallback(const google::protobuf::MethodDescriptor* method) {
  server_methods.erase(method->full_name());
  ServiceMethodLocator::Instance()->DeregisterMethod(fancy, method);
}

TEST(ServiceMethodLocator, ServerSide) {
  ServiceMethodLocator::Instance()->RegisterMethodProvider(
      ServerAddCallback, ServerRemoveCallback);
  ServiceMethodLocator::Instance()->AddService(service);
  std::set<std::string> expected;
  for (int i = 0; i != service->method_count(); ++i) {
    expected.insert(service->method(i)->full_name());
  }
  ASSERT_EQ(expected, server_methods);
  ASSERT_EQ(service->method(0)->full_name(),
            ServiceMethodLocator::Instance()
                ->TryGetMethodDesc(fancy, service->method(0))
                ->normalized_method_name);
  ServiceMethodLocator::Instance()->DeleteService(service);
  ASSERT_TRUE(server_methods.empty());
}

}  // namespace tinyRPC::protobuf

TINYRPC_TEST_MAIN
