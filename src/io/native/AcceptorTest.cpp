#include "Acceptor.h"

#include <thread>
#include <utility>

#include "../../include/gtest/gtest.h"
#include "../../include/glog/logging.h"

#include "../EventLoop.h"
#include "../util/Socket.h"
#include "../../testing/main.h"
#include "../../testing/Endpoint.h"

using namespace std::literals;

namespace tinyRPC {

TEST(NativeAcceptor, All) {
  // Default of `net.core.somaxconn`.
  //
  // @sa: https://serverfault.com/q/518862
  constexpr auto kConnectAttempts = 128;

  std::atomic<int> conns = 0;
  auto addr = testing::PickAvailableEndpoint();
  // backlog must be large enough for connections below (made in burst) to
  // succeeded.
  auto listen_fd = io::util::CreateListener(addr, kConnectAttempts);
  CHECK(listen_fd);
  NativeAcceptor::Options opts;
  opts.connection_handler = [&](Handle fd, const Endpoint& peer) { ++conns; };
  io::util::SetNonBlocking(listen_fd.Get());
  io::util::SetCloseOnExec(listen_fd.Get());
  auto acceptor = std::make_shared<NativeAcceptor>(std::move(listen_fd), std::move(opts));
  GetGlobalEventLoop(0)->AttachDescriptor(acceptor);

  Handle clients[kConnectAttempts];
  for (int i = 0; i != kConnectAttempts; ++i) {
    clients[i] = io::util::CreateStreamSocket(addr.Get()->sa_family);
    io::util::SetNonBlocking(clients[i].Get());
    io::util::SetCloseOnExec(clients[i].Get());
    io::util::StartConnect(clients[i].Get(), addr);
  }
  std::cout << "All connection attempts made." << std::endl;
  while (conns != kConnectAttempts) {
    std::this_thread::sleep_for(1ms);
  }
  ASSERT_EQ(kConnectAttempts, conns.load());
  acceptor->Stop();
  acceptor->Join();
}

}  // namespace tinyRPC

TINYRPC_TEST_MAIN
