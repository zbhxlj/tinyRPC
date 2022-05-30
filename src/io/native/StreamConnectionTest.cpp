#include "StreamConnection.h"

#include <sys/signal.h>

#include <ios>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "../../include/gtest/gtest.h"

#include "../../base/chrono.h"
#include "../../base/String.h"
#include "../../fiber/ThisFiber.h"
#include "../EventLoop.h"
#include "Acceptor.h"
#include "../util/Socket.h"
#include "../../testing/Endpoint.h"
#include "../../testing/main.h"
#include "../../base/MaybeOwning.h"

using namespace std::literals;

namespace tinyRPC {
std::atomic<int> closed {0};
std::atomic<int> err {0};

class ConnectionHandler : public StreamConnectionHandler {
 public:
  using Callback = UniqueFunction<DataConsumptionStatus(std::string&)>;

  explicit ConnectionHandler(std::string name, Callback cb)
      : name_(std::move(name)), cb_(std::move(cb)) {}

  void OnAttach(StreamConnection*) override {}
  void OnDetach() override {}

  void OnWriteBufferEmpty() override {}
  void OnDataWritten(std::uintptr_t ctx) override {}

  DataConsumptionStatus OnDataArrival(std::string& buffer) override {
    return cb_(buffer);
  }

  void OnClose() override {}
  void OnError() override { CHECK(!"Unexpected."); }

 private:
  std::string name_;
  Callback cb_;
};

class ClosedConnectionHandler : public StreamConnectionHandler {
 public:
  void OnAttach(StreamConnection*) override {}
  void OnDetach() override {}
  void OnWriteBufferEmpty() override { CHECK(!"Unexpected."); }
  void OnDataWritten(std::uintptr_t ctx) override { CHECK(!"Unexpected."); }
  DataConsumptionStatus OnDataArrival(std::string& buffer) override {
    CHECK(!"Unexpected.");
  }
  void OnClose() override { ++closed; }
  void OnError() override { CHECK(!"Unexpected."); };

};

class ErrorConnectionHandler : public StreamConnectionHandler {
 public:
  void OnAttach(StreamConnection*) override {}
  void OnDetach() override {}
  void OnWriteBufferEmpty() override { CHECK(!"Unexpected."); }
  void OnDataWritten(std::uintptr_t ctx) override { CHECK(!"Unexpected."); }
  DataConsumptionStatus OnDataArrival(std::string& buffer) override {
    CHECK(!"Unexpected.");
  }
  void OnClose() override { CHECK(!"Unexpected."); }
  void OnError() override { ++err; };

};

class NativeStreamConnectionTest : public ::testing::Test {
 public:
  void SetUp() override {
    conns_ = 0;
    // backlog must be large enough for connections below (made in burst) to
    // succeeded.
    auto listen_fd = io::util::CreateListener(addr_, kConnectAttempts);
    CHECK(listen_fd);
    NativeAcceptor::Options opts;
    // A simple echo server.
    opts.connection_handler = [&](Handle fd, const Endpoint& peer) {
      auto index = conns_++;
      if (!accept_conns) {
        std::cout << "Rejecting connection." << std::endl;
        // Refuse further connections.
        return;
      }
      CHECK_LT(index, kConnectAttempts);

      io::util::SetNonBlocking(fd.Get());
      io::util::SetCloseOnExec(fd.Get());
      NativeStreamConnection::Options opts;
      opts.read_buffer_size = 11111;
      opts.handler = std::make_unique<ConnectionHandler>(
          Format("server handler {}", index),
          [this, index](std::string& buffer) {
            if(buffer != std::string("hello")){
              LOG(INFO) << "Not eq" << "\n";
            }
            LOG(INFO) << "Buffer = " << buffer << " Size = " << buffer.size() << "\n";
            server_conns_[index]->Write(buffer, 0);
            return StreamConnectionHandler::DataConsumptionStatus::Ready;
          });
      server_conns_[index] = std::make_shared<NativeStreamConnection>(
          std::move(fd), std::move(opts));
      GetGlobalEventLoop(0)
          ->AttachDescriptor(std::dynamic_pointer_cast<Descriptor>(server_conns_[index]));
      server_conns_[index]->StartHandshaking();
    };
    io::util::SetNonBlocking(listen_fd.Get());
    io::util::SetCloseOnExec(listen_fd.Get());
    acceptor_ =
        std::make_shared<NativeAcceptor>(std::move(listen_fd), std::move(opts));
    GetGlobalEventLoop(0)->AttachDescriptor(std::dynamic_pointer_cast<Descriptor>(acceptor_));
  }

  void TearDown() override {
    acceptor_->Stop();
    acceptor_->Join();
    for (auto&& e : server_conns_) {
      if (e) {
        e->Stop();
        e->Join();
        e = nullptr;
      }
    }
  }

 protected:
  // Default of `net.core.somaxconn`.
  static constexpr auto kConnectAttempts = 128;
  bool accept_conns = true;
  std::atomic<int> conns_{0};
  Endpoint addr_ = testing::PickAvailableEndpoint();
  std::shared_ptr<NativeAcceptor> acceptor_;
  std::shared_ptr<NativeStreamConnection> server_conns_[kConnectAttempts];
};

TEST_F(NativeStreamConnectionTest, Echo) {
  static const std::string kData = "hello";
  std::atomic<int> replied{0};
  std::shared_ptr<NativeStreamConnection> clients[kConnectAttempts];
  for (int i = 0; i != kConnectAttempts; ++i) {
    auto fd = io::util::CreateStreamSocket(addr_.Family());
    io::util::SetNonBlocking(fd.Get());
    io::util::SetCloseOnExec(fd.Get());
    io::util::StartConnect(fd.Get(), addr_);
    NativeStreamConnection::Options opts;
    opts.handler = std::make_unique<ConnectionHandler>(
        Format("client handler {}", i), [&](std::string& buffer) {
          if (buffer.size() != kData.size()) {
            return StreamConnectionHandler::DataConsumptionStatus::Ready;
          }
          [&] { ASSERT_EQ(kData, buffer); }();
          ++replied;
          return StreamConnectionHandler::DataConsumptionStatus::Ready;
        });
    opts.read_buffer_size = 111111;
    clients[i] =
        std::make_shared<NativeStreamConnection>(std::move(fd), std::move(opts));
    GetGlobalEventLoop(0)->AttachDescriptor(std::dynamic_pointer_cast<Descriptor>(clients[i]));
    clients[i]->StartHandshaking();
    clients[i]->Write(kData, 0);
  }
  while (replied != kConnectAttempts) {
    std::this_thread::sleep_for(100ms);
  }
  ASSERT_EQ(kConnectAttempts, replied.load());
  for (auto&& c : clients) {
    c->Stop();
    c->Join();
  }
}

TEST_F(NativeStreamConnectionTest, EchoWithHeavilyFragmentedBuffer) {
  std::string buffer;
  for (int i = 0; i != 600; ++i) {
    buffer.append(std::string(1, i % 26 + 'a'));
  }

  std::shared_ptr<NativeStreamConnection> client;
  std::string received;
  std::atomic<int> bytes_received{};
  auto fd = io::util::CreateStreamSocket(addr_.Family());
  io::util::SetNonBlocking(fd.Get());
  io::util::SetCloseOnExec(fd.Get());
  io::util::StartConnect(fd.Get(), addr_);
  NativeStreamConnection::Options opts;
  opts.handler =
      std::make_unique<ConnectionHandler>("", [&](std::string& buffer) {
        bytes_received += buffer.size();
        received += buffer;
        buffer.clear();
        return StreamConnectionHandler::DataConsumptionStatus::Ready;
      });
  opts.read_buffer_size = 111111;
  client =
      std::make_shared<NativeStreamConnection>(std::move(fd), std::move(opts));
  GetGlobalEventLoop(0)->AttachDescriptor(std::dynamic_pointer_cast<Descriptor>(client));
  client->StartHandshaking();
  client->Write(buffer, 0);
  while (bytes_received != buffer.size()) {
    std::this_thread::sleep_for(100ms);
  }
  ASSERT_EQ(buffer, received);
  client->Stop();
  client->Join();
}

// TODO: Why this will close?
TEST_F(NativeStreamConnectionTest, RemoteClose) {
  accept_conns = false;
  ClosedConnectionHandler cch;
  NativeStreamConnection::Options opts;
  opts.handler = MaybeOwning(non_owning, &cch);
  opts.read_buffer_size = 111111;
  auto fd = io::util::CreateStreamSocket(addr_.Get()->sa_family);
  io::util::SetNonBlocking(fd.Get());
  io::util::SetCloseOnExec(fd.Get());
  io::util::StartConnect(fd.Get(), addr_);
  auto sc =
      std::make_shared<NativeStreamConnection>(std::move(fd), std::move(opts));
  GetGlobalEventLoop(0)->AttachDescriptor(sc);
  sc->StartHandshaking();
  std::this_thread::sleep_for(100ms);
  EXPECT_EQ(1, closed.load());
  sc->Stop();
  sc->Join();
}

TEST(NativeStreamConnection, ConnectionFailure) {
  ErrorConnectionHandler ech;
  NativeStreamConnection::Options opts;
  opts.handler = MaybeOwning(non_owning, &ech);
  opts.read_buffer_size = 111111;
  auto fd = io::util::CreateStreamSocket(AF_INET);
  // Hopefully no one is listening there.
  auto invalid = EndpointFromIpv4("127.0.0.1", 1);
  io::util::SetNonBlocking(fd.Get());
  io::util::SetCloseOnExec(fd.Get());
  io::util::StartConnect(fd.Get(), invalid);
  auto sc =
      std::make_shared<NativeStreamConnection>(std::move(fd), std::move(opts));
  GetGlobalEventLoop(0)->AttachDescriptor(sc);
  sc->StartHandshaking();
  std::this_thread::sleep_for(100ms);
  ASSERT_EQ(1, err.load());
  sc->Stop();
  sc->Join();
}

TEST(NativeStreamConnection, NoBandwidthLimit) {
  constexpr auto kBodySize = 64 * 1024 * 1024;
  auto addr = testing::PickAvailableEndpoint();

  // Server side.
  std::atomic<std::size_t> received = 0;
  std::shared_ptr<NativeStreamConnection> server_conn;
  auto acceptor = [&] {
    auto listen_fd = io::util::CreateListener(addr, 100);
    CHECK(listen_fd);
    NativeAcceptor::Options opts;
    opts.connection_handler = [&](Handle fd, const Endpoint& peer) {
      io::util::SetNonBlocking(fd.Get());
      io::util::SetCloseOnExec(fd.Get());
      NativeStreamConnection::Options opts;
      opts.read_buffer_size = kBodySize;
      opts.handler = std::make_unique<ConnectionHandler>(
          Format("server handler "), [&](std::string& buffer) {
            received += buffer.size();
            buffer.clear();  // All consumed.
            return StreamConnectionHandler::DataConsumptionStatus::Ready;
          });
      server_conn = std::make_shared<NativeStreamConnection>(std::move(fd),
                                                           std::move(opts));
      GetGlobalEventLoop(0)
          ->AttachDescriptor(server_conn);
      server_conn->StartHandshaking();
    };
    io::util::SetNonBlocking(listen_fd.Get());
    io::util::SetCloseOnExec(listen_fd.Get());
    return std::make_shared<NativeAcceptor>(std::move(listen_fd),
                                            std::move(opts));
  }();
  GetGlobalEventLoop(0)->AttachDescriptor(std::dynamic_pointer_cast<Descriptor>(acceptor));

  // Client side.
  auto client_conn = [&] {
    auto fd = io::util::CreateStreamSocket(addr.Family());
    io::util::SetNonBlocking(fd.Get());
    io::util::SetCloseOnExec(fd.Get());
    io::util::StartConnect(fd.Get(), addr);
    NativeStreamConnection::Options opts;
    opts.handler = std::make_unique<ConnectionHandler>(
        Format("client handler"), [&](std::string& buffer) {
          CHECK(!"Nothing should be echo-d back.");
          return StreamConnectionHandler::DataConsumptionStatus::Ready;
        });
    opts.read_buffer_size = kBodySize;
    return std::make_shared<NativeStreamConnection>(std::move(fd),
                                                  std::move(opts));
  }();
  GetGlobalEventLoop(0)->AttachDescriptor(std::dynamic_pointer_cast<Descriptor>(client_conn));
  client_conn->StartHandshaking();
  auto start = ReadSteadyClock();
  client_conn->Write(std::string(kBodySize, 1), 0);
  while (received.load() != kBodySize) {
    this_fiber::SleepFor(1ms);
  }
  auto time_use = ReadSteadyClock() - start;

  ASSERT_LE(time_use / 1ms, 10s / 1ms);  // 10s should be far more than enough.
  acceptor->Stop();
  acceptor->Join();
  server_conn->Stop();
  server_conn->Join();
  client_conn->Stop();
  client_conn->Join();
}

}  // namespace tinyRPC

TINYRPC_TEST_MAIN
