#include "rpcControllerServer.h"

#include <sys/signal.h>

#include <chrono>
#include <thread>

#include "gtest/gtest.h"

#include "../../../base/Logging.h"
#include "../../../fiber/ThisFiber.h"
#include "rpcChannel.h"
#include "rpcControllerClient.h"
#include "../../Server.h"
#include "../../../testing/echo_service.pb.h"
#include "../../../testing/Endpoint.h"
#include "../../../testing/main.h"

using namespace std::literals;

namespace tinyRPC {

class DummyEcho : public testing::EchoService {
 public:
  void Echo(RpcServerController* controller,
                       const ::tinyRPC::testing::EchoRequest* request,
                       ::tinyRPC::testing::EchoResponse* response,
                       ::google::protobuf::Closure* done) override {
    if (test_cb_) {
      test_cb_(request, response, controller);
    }

    auto now = ReadSteadyClock();
    response->set_body(request->body());
    controller->WriteResponseImmediately();

    this_fiber::SleepFor(1s);
  }
  UniqueFunction<void(const testing::EchoRequest&, testing::EchoResponse*,
                RpcServerController*)> test_cb_;
} dummy;

TEST(RpcServerController, Basics) {
  RpcServerController rc;

  rc.SetFailed("failed");
  ASSERT_TRUE(rc.Failed());
  ASSERT_EQ("failed", rc.ErrorText());
}

TEST(RpcServerController, Timeout) {
  RpcServerController ctlr;
  ASSERT_FALSE(ctlr.GetTimeout());
  ctlr.SetTimeout(ReadSteadyClock() + 1s);
  ASSERT_TRUE(ctlr.GetTimeout());
  EXPECT_NEAR((*ctlr.GetTimeout() - ReadSteadyClock()) / 1ms, 1s / 1ms, 100);
  ctlr.Reset();
  ASSERT_FALSE(ctlr.GetTimeout());
}

class RpcServerControllerTest : public ::testing::Test {
  void SetUp() override {
    listening_on_ = testing::PickAvailableEndpoint();
    server_ = std::make_unique<Server>();
    server_->AddService(&dummy);
    server_->AddProtocol("flare");
    server_->ListenOn(listening_on_);
    server_->Start();

    channel_ = std::make_unique<RpcChannel>();
    channel_->Open("flare://" + listening_on_.ToString());

    stub_ = std::make_unique<testing::EchoService_Stub>(channel_.get());
  }

  void TearDown() override {
    server_->Stop();
    server_->Join();
    server_.reset();
    channel_.reset();
    stub_.reset();
  }

 protected:
  Endpoint listening_on_;
  std::unique_ptr<Server> server_;
  std::unique_ptr<RpcChannel> channel_;
  std::unique_ptr<testing::EchoService_Stub> stub_;
};

TEST_F(RpcServerControllerTest, Timestamps) {
  testing::EchoRequest req;

  RpcClientController ctlr;
  testing::EchoResponse res;
  req.set_body("aaa");
  stub_->Echo(&ctlr, &req, &res, nullptr);
  ASSERT_EQ("aaa", res.body());
}


// TEST_F(RpcServerControllerTest, TimeoutFromClient) {
//   testing::EchoRequest req;

//   // No timeout is provided. The default 2s timeout is applied.
//   dummy.test_cb_ = [&](auto&&, auto&&, RpcServerController* ctlr) {
//     ASSERT_TRUE(ctlr->GetTimeout());
//     EXPECT_NEAR((*ctlr->GetTimeout() - ReadSteadyClock()) / 1ms, 2s / 1ms, 100);
//   };

//   RpcClientController ctlr;
//   req.set_body("aaa");
//   stub_->Echo(req, &ctlr);
//   ASSERT_EQ("aaa", ->body());

// }

}  // namespace tinyRPC

TINYRPC_TEST_MAIN
