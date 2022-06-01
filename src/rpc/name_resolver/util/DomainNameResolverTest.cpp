#include "DomainNameResolver.h"

#include "gtest/gtest.h"

namespace tinyRPC::name_resolver::util {

TEST(ResolveDomain, ResolveDomain) {
  std::vector<Endpoint> addresses;
  if (ResolveDomain("www.qq.com", 443, &addresses)) {
    std::cout << "Query success:\nIP:";
    for (size_t i = 0; i < addresses.size(); ++i)
      std::cout << " " << addresses[i].ToString();
    std::cout << "\n";
  } else {
    std::cout << "Query error: \n";
  }
}

TEST(ResolveDomain, Invalid) {
  std::vector<Endpoint> addresses;
  EXPECT_FALSE(
      ResolveDomain("non-exist", 0, &addresses));
  EXPECT_FALSE(
      ResolveDomain("non-exist.domain", 0, &addresses));
}

TEST(ResolveDomain, ResolveWithServers) {
  std::vector<Endpoint> addresses;
  ASSERT_TRUE(
      ResolveDomain("example.com", 1234 , &addresses));
}

}  // namespace tinyRPC::name_resolver::util
