#include "List.h"
#include "util/DomainNameResolver.h"

#include "gtest/gtest.h"

namespace tinyRPC::name_resolver {

TEST(ListNameResolver, StartResolving) {
  auto list_name_resolver_factory = name_resolver_registry.Get("list");
  ASSERT_TRUE(!!list_name_resolver_factory);
  auto list_view = list_name_resolver_factory->StartResolving(
      "192.0.2.1.5:0,192.0.2.2:8080");
  ASSERT_FALSE(!!list_view);
  list_view = list_name_resolver_factory->StartResolving(
      "192.0.2.1:80,192.0.2.2:8080,[2001:db8::1]:8088");
  ASSERT_TRUE(!!list_view);
  EXPECT_EQ(1, list_view->GetVersion());
  std::vector<Endpoint> peers;
  list_view->GetPeers(&peers);
  ASSERT_EQ(3, peers.size());
  ASSERT_EQ("192.0.2.1:80", peers[0].ToString());
  ASSERT_EQ("192.0.2.2:8080", peers[1].ToString());
  ASSERT_EQ("[2001:db8::1]:8088", peers[2].ToString());
}

}  // namespace tinyRPC::name_resolver
