#include "EventLoop.h"

#include <chrono>

#include "../../include/gtest/gtest.h"

#include "../testing/main.h"

using namespace std::literals;

namespace tinyRPC {

TEST(EventLoop, Task) {
  std::atomic<int> x = 0;
  GetGlobalEventLoop(0)->AddTask([&] { x = 1; });
  while (x == 0) {
  }
  ASSERT_EQ(1, x);
}

}  // namespace tinyRPC

TINYRPC_TEST_MAIN
