#ifndef _SRC_RPC_INTERNAL_NORMAL_CONNECTION_HANDLER_H_
#define _SRC_RPC_INTERNAL_NORMAL_CONNECTION_HANDLER_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ServerConnectionHandler.h"
#include "../protocol/StreamProtocol.h"
#include "../protocol/StreamService.h"
#include "../../fiber/Mutex.h"

namespace tinyRPC {

class Server;

}  // namespace flare

namespace tinyRPC::rpc::detail {

class NormalConnectionHandler : public ServerConnectionHandler {
 public:
  struct Context {
    std::uint64_t id;
    std::string service_name;
    Endpoint local_peer;
    Endpoint remote_peer;
    std::vector<std::unique_ptr<StreamProtocol>> protocols;
    std::vector<StreamService*> services;

    // From corresponding field in `Server::Options`.
    std::chrono::nanoseconds max_request_queueing_delay{};
  };

  NormalConnectionHandler(Server* owner, std::unique_ptr<Context> ctx);

  void Stop() override;
  void Join() override;

 private:
  // We do not care about these events.
  void OnAttach(StreamConnection* conn) override;
  void OnDetach() override;
  void OnWriteBufferEmpty() override;
  void OnDataWritten(std::uintptr_t ctx) override;

  DataConsumptionStatus OnDataArrival(std::string& buffer) override;

  void OnClose() override;
  void OnError() override;

 private:
  enum class ProcessingStatus { Success, Error, Saturated, SuppressRead };

  ProcessingStatus ProcessOnePacket(std::string& buffer,
                                    std::uint64_t receive_tsc);

  // This method try cutting off a message using the protocol that had
  // succeeded.
  StreamProtocol::MessageCutStatus TryCutMessageUsingLastProtocol(
      std::string& buffer, std::unique_ptr<Message>* msg,
      StreamProtocol** used_protocol);

  // The method cut off a message without parsing it, to release CPU as soon as
  // possible.
  ProcessingStatus TryCutMessage(std::string& buffer,
                                 std::unique_ptr<Message>* msg,
                                 StreamProtocol** used_protocol);

  std::unique_ptr<Controller> NewController(const Message& message,
                                            StreamProtocol* protocol) const;

  // Write an "overloaded" response.
  void WriteOverloaded(const Message& corresponding_req,
                       StreamProtocol* protocol, Controller* controller);

  // Serialize message and write it out.
  std::size_t WriteMessage(const Message& msg, StreamProtocol* protocol,
                           Controller* controller, std::uintptr_t ctx) const;

  // This method is executed in dedicated fiber, so blocking does not matter
  // much.
  //
  // Any (unrecoverable) failure in this method leads to packet drop.
  void ServiceFastCall(std::unique_ptr<Message>&& msg, StreamProtocol* protocol,
                       std::unique_ptr<Controller> controller,
                       std::uint64_t receive_tsc, std::size_t pkt_size);

  // In case the server is overloaded, both fast call & streaming call messages
  // are handled by this method.
  void ServiceOverloaded(std::unique_ptr<Message> msg, StreamProtocol* protocol,
                         Controller* controller);

  StreamService* FindAndCacheMessageHandler(
      const Message& message, const tinyRPC::Controller& controller,
      StreamService::InspectionResult* inspection_result);
  StreamService* FindAndCacheMessageHandlerSlow(
      const Message& message, const tinyRPC::Controller& controller,
      StreamService::InspectionResult* inspection_result);

  // Prepare any prerequisite fiber context for new RPC and call `cb`.
  template <class F>
  void PrepareForRpc(const StreamService::InspectionResult& inspection_result,
                     const Controller& controller, F&& cb);

  // Wait until the RPC has fully completed.
  //
  // This method usually immediately returns. However, if the user did some
  // fire-and-forget operation, this method wait until all those (possibly
  // asynchronous) operations finished.
  void WaitForRpcCompletion();


  // Called upon new RPC arrival. For streaming RPC, this is only called for the
  // first message in the stream.
  bool OnNewCall();

  // Called when RPC has finished.
  void OnCallCompletion();


 private:
  // Stream calls cannot use this ID as their call ID. We need this id to speed
  // up `OnDataWritten` callback.
  static constexpr auto kFastCallReservedContextId = 0xffff'ffff'ffff'ffffULL;


  Server* owner_;
  std::unique_ptr<Context> ctx_;
  StreamConnection* conn_{};

  // This flag is set if we've ever succeeded in parsing at least on message.
  //
  // In this case, if we we get `NotIdentified`, we don't try other
  // protocol (for better performance).
  bool ever_succeeded_cut_msg_{false};

  // This is the index of protocol last successfully cut off a message from
  // wire. We'll prefer it the next time a packet arrived.
  std::size_t last_protocol_{0};

  // The last service successfully handled the request.
  //
  // Since it's modified by worker fibers (where we try services), it's marked
  // as atomic.
  std::atomic<StreamService*> last_service_;

  // Unfinished calls to services.
  std::atomic<std::size_t> ongoing_requests_{0};

  // Only stream calls' context is saved here. For fast calls they're largely
  // stateless from our perspective.
  //
  // Do NOT use `std::mutex` here. we'll be calling methods that trigger fiber
  // scheduler under the lock (i.e., the lock will be locked & unlocked in
  // different pthread worker, albeit in the same fiber.).
  fiber::Mutex lock_;

};

}  // namespace tinyRPC::rpc::detail

#endif  
