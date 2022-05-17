#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "../../include/gtest/gtest.h"

#include "Latch.h"
#include "Testing.h"
#include "Fiber.h"
#include "ThisFiber.h"

using namespace std::literals;

namespace tinyRPC::fiber {

std::atomic<bool> exiting{false};

void RunTest() {
  std::atomic<std::size_t> local_count {0}, remote_count {0};
  while (!exiting) {
    Latch l(1);
    auto called = std::make_shared<std::atomic<bool>>(false);
    Fiber([called, &l, &remote_count] {
      if (!called->exchange(true)) {
        this_fiber::Yield();
        l.count_down();
        ++remote_count;
      }
    }).detach();
    this_fiber::Yield();
    if (!called->exchange(true)) {
      l.count_down();
      ++local_count;
    }
    l.wait();
  }
  std::cout << local_count << " " << remote_count << std::endl;
}

TEST(Latch, Torture) {
  testing::RunAsFiber([] {
    Fiber fs[10];
    for (auto&& f : fs) {
      f = Fiber(RunTest);
    }
    std::this_thread::sleep_for(3s);
    exiting = true;
    for (auto&& f : fs) {
      f.join();
    }
  });
}

TEST(Latch, CountDownTwo) {
  testing::RunAsFiber([] {
    Latch l(2);
    l.arrive_and_wait(2);
    ASSERT_TRUE(1);
  });
}

TEST(Latch, WaitFor) {
  testing::RunAsFiber([] {
    Latch l(1);
    ASSERT_FALSE(l.wait_for(1s));
    l.count_down();
    ASSERT_TRUE(l.wait_for(0ms));
  });
}

TEST(Latch, WaitUntil) {
  testing::RunAsFiber([] {
    Latch l(1);
    ASSERT_FALSE(l.wait_until(std::chrono::steady_clock::now() + 1s));
    l.count_down();
    ASSERT_TRUE(l.wait_until(std::chrono::steady_clock::now()));
  });
}

}  // namespace tinyRPC::fiber
