#include "Descriptor.h"

#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <thread>
#include <utility>

#include "../../include/gtest/gtest.h"

#include "../base/Random.h"
#include "../fiber/Fiber.h"
#include "../fiber/Latch.h"
#include "../fiber/ThisFiber.h"
#include "EventLoop.h"
#include "../testing/main.h"

using namespace std::literals;

namespace tinyRPC {

std::atomic<std::size_t> cleaned{};

class PipeDesc : public Descriptor {
 public:
  explicit PipeDesc(Handle handle)
      : Descriptor(std::move(handle), Event::Write) {}
  EventAction OnReadable() override { return read_rc_; }
  EventAction OnWritable() override { return EventAction::Ready; }
  void OnError(int err) override {}
  void OnCleanup(CleanupReason reason) override { ++cleaned; }

  void SetReadAction(EventAction act) { read_rc_ = act; }

 private:
  EventAction read_rc_;
};

std::shared_ptr<PipeDesc> CreatePipe() {
  int fds[2];
  PCHECK(pipe2(fds, O_NONBLOCK | O_CLOEXEC) == 0);
  PCHECK(write(fds[1], "asdf", 4) == 4);
  close(fds[1]);
  return std::make_shared<PipeDesc>(Handle(fds[0]));
}

TEST(Descriptor, ConcurrentRestartRead) {
  using namespace tinyRPC::fiber;
  for (auto&& action :
       {Descriptor::EventAction::Ready, Descriptor::EventAction::Suppress}) {
    for (int i = 0; i != 1000; ++i) {
      Fiber fibers[2];
      fiber::Latch latch(1);
      auto desc = CreatePipe();
      auto ev = GetGlobalEventLoop(0);

      desc->SetReadAction(action);
      ev->AttachDescriptor(desc, true);

      fibers[0] = Fiber([desc, &latch] {
        latch.wait();
        desc->RestartReadIn(0ns);
      });
      fibers[1] = Fiber([desc, &latch] {
        latch.wait();
        desc->Kill(Descriptor::CleanupReason::Closing);
      });
      latch.count_down();
      for (auto&& e : fibers) {
        e.join();
      }
    }
  }
  while (cleaned != 1000 * 2) {
    this_fiber::SleepFor(1ms);
  }
}

}  // namespace tinyRPC

TINYRPC_TEST_MAIN
