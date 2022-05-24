#include "Socket.h"

#include <sys/types.h>
#include <unistd.h>

#include "../../include/gtest/gtest.h"

#include "../../testing/Endpoint.h"
#include "../../testing/main.h"

using namespace std::literals;

namespace tinyRPC::io::util {

TEST(Socket, CreateListener) {
  auto addr = testing::PickAvailableEndpoint();
  auto fd1 = CreateListener(addr, 10);
  ASSERT_TRUE(fd1);

  if (getuid() != 0) {
    // Shouldn't success as non-root should not be able to listen on port number
    // less than 1024.
    auto addr2 = EndpointFromIpv4("127.0.0.1", 1);
    auto fd2 = CreateListener(addr2, 10);
    ASSERT_FALSE(fd2);
  }
}

}  // namespace tinyRPC::io::util

TINYRPC_TEST_MAIN
