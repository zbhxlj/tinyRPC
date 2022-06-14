#include "Server.h"

#include <fstream>
#include <string>
#include <utility>

#include "../base/chrono.h"
#include "../base/ScopedDeferred.h"
#include "../fiber/Runtime.h"
#include "../fiber/ThisFiber.h"
#include "../io/EventLoop.h"
#include "../io/native/Acceptor.h"
#include "../io/native/StreamConnection.h"
#include "../io/util/Socket.h"
#include "internal/NormalConnectionHandler.h"
#include "protocol/protobuf/service.h"
#include "protocol/StreamProtocol.h"
#include "protocol/StreamService.h"

DEFINE_int32(flare_rpc_server_max_ongoing_calls, 10000,
             "Maximum number of unfinished calls. After this limit is reached, "
             "new calls are dropped unless an old one has finished.");
DEFINE_int32(flare_rpc_server_max_connections, 10000,
             "Maximum concurrent incoming connections. Once reached, new "
             "connection requests are rejected.");
DEFINE_int32(flare_rpc_server_max_request_queueing_delay, 0,
             "Maximum number of milliseconds a request can be delayed (in some "
             "sort of queues) before being processed. Any requests delayed "
             "longer is rejected. Setting it to zero disables this behavior.");
DEFINE_int32(flare_rpc_server_max_packet_size, 4 * 1024 * 1024,
             "Default maximum packet size of `Server`.");
DEFINE_int32(flare_rpc_server_remove_idle_connection_interval, 15,
             "Interval, in seconds, between to run of removing idle "
             "server-side connections.");
DEFINE_int32(
    flare_rpc_server_connection_max_idle, 60,
    "Time period before recycling a server-side idle connection, in seconds.");
DEFINE_bool(flare_rpc_server_suppress_ephemeral_port_warning, false,
            "If set, no warning will be printed when ephemeral port is used "
            "for serving RPC. This is mostly used by UTs.");
            
using namespace std::literals;

namespace tinyRPC {

namespace {

// Tests if `port` is unsafe to be used as a serving port.
bool IsPortUnsafeForServingV4(std::uint16_t port) {
  std::uint16_t since, upto;
  std::ifstream ifs("/proc/sys/net/ipv4/ip_local_port_range");
  ifs >> since >> upto;
  if (!ifs) {  // Cannot determine if `port` is unsafe, be conservative and
               // don't print a warning then.
    return false;
  }
  return port >= since && port <= upto;
}

}  // namespace

struct Server::ConnectionContext {
  std::uint64_t scheduling_group_id;
  std::uint64_t conn_id;
  std::shared_ptr<NativeStreamConnection> conn;
  Endpoint remote_peer;
  rpc::detail::ServerConnectionHandler* handler;
};

Server::Server() : Server(Options()) {}

Server::Server(Options options)
    : options_(std::move(options)) {

  idle_conn_cleaner_ = fiber::SetTimer(
      ReadSteadyClock(),
      FLAGS_flare_rpc_server_remove_idle_connection_interval * 1s,
      [this] { OnConnectionCleanupTimer(); });
}

Server::~Server() {
  if (state_ == ServerState::Initialized || state_ == ServerState::Joined) {
    // Nothing to do then.
  } else if (state_ == ServerState::Running) {
    Stop();
    Join();
  } else {
    FLARE_CHECK(state_ == ServerState::Stopped);
    // Given that the user explicitly called `Stop()`, not calling `Join` is
    // likely a programming error. So we raise here.
    FLARE_LOG_FATAL(
        "You should either: 1) call both Stop() and Join(), or 2) not call any "
        "of them (in which case they are called implicitly on destruction). "
        "Only calling `Stop()` but not `Join()` is treated as a programming "
        "error.");
  }
}

void Server::AddProtocol(const std::string& name) {
  if (known_protocols_.find(name) == known_protocols_.end()) {
    AddProtocol(server_side_stream_protocol_registry.GetFactory(name));
    known_protocols_.insert(name);
  }
}

void Server::AddProtocol(Factory<StreamProtocol> factory) {
  protocol_factories_.push_back(std::move(factory));
}

void Server::AddProtocols(const std::vector<std::string>& names) {
  for (auto&& e : names) {
    AddProtocol(e);
  }
}

void Server::AddService(
    MaybeOwningArgument<google::protobuf::Service> service) {
  GetBuiltinNativeService<protobuf::Service>()->AddService(std::move(service));
}

void Server::AddNativeService(MaybeOwningArgument<StreamService> service) {
  services_.push_back(std::move(service));
}

void Server::ListenOn(const Endpoint& addr, int backlog) {
  // We might want to lift this restriction if there's a need to providing same
  // service(s) on multiple ports. (To switch server port for whatever reason,
  // for example.)
  FLARE_CHECK(!listen_cb_,
              "Calling `ListenOn` for multiple times is not allowed.");
  // It's advisable not to use ephemeral port for serving RPCs. Print a warning
  // log if the user intend to.
  FLARE_LOG_WARNING_IF(
      addr.Family() == AF_INET &&
          IsPortUnsafeForServingV4(EndpointGetPort(addr)) &&
          !FLAGS_flare_rpc_server_suppress_ephemeral_port_warning,
      "Using ephemeral port [{}] to serve requests. This is generally "
      "considered unsafe as the system may allocate this port to other process "
      "for outgoing connection before your program starts. If that is the "
      "case, your program won't start successfully. You can safely ignore this "
      "warning for UTs.",
      EndpointGetPort(addr));
  listening_on_ = addr;
  listen_cb_ = [=] {
    // Create listening socket.
    auto fd = io::util::CreateListener(addr, backlog);
    FLARE_CHECK(!!fd, "Cannot create listener.");
    io::util::SetNonBlocking(fd.Get());
    io::util::SetCloseOnExec(fd.Get());
    io::util::SetTcpNoDelay(fd.Get());

    // In fact we start listening once `ListenOn` is called (instead of on
    // `Start()`'s return.)
    NativeAcceptor::Options opts;
    opts.connection_handler = [this](auto&&... args) {
      return OnConnection(std::move(args)...);
    };
    acceptor_ = std::make_shared<NativeAcceptor>(std::move(fd), std::move(opts));
    // TODO(luobogao): Duplicate fd and create several acceptors, one for each
    // worker group.
  };
}

bool Server::Start() {
  FLARE_CHECK(state_ == ServerState::Initialized,
              "`Start` may only be called once.");
  state_ = ServerState::Running;


  FLARE_CHECK(!!listen_cb_, "You haven't called `ListenOn` yet.");
  listen_cb_();

  GetGlobalEventLoop(0)->AttachDescriptor(acceptor_);
  return true;
}

void Server::Stop() {
  FLARE_CHECK(state_ == ServerState::Running,
              "The server has not been started yet.");
  state_ = ServerState::Stopped;

  // No longer necessary as we're going to leave anyway.
  fiber::KillTimer(idle_conn_cleaner_);

  // We're no longer interested in accepting new connections.
  acceptor_->Stop();
}

void Server::Join() {
  FLARE_CHECK(state_ == ServerState::Stopped,
              "The server must be stopped before joining it.");
  state_ = ServerState::Joined;

  // Make sure no new connection will come first.
  acceptor_->Join();

  // Now we're safe to close existing connections.
  std::unordered_map<std::uint64_t, std::unique_ptr<ConnectionContext>>
      conns_temp;
  {
    std::scoped_lock lk(conns_lock_);
    conns_temp.swap(conns_);
  }
  for (auto&& [_, c] : conns_temp) {
    c->conn->Stop();
  }
  for (auto&& [_, c] : conns_temp) {
    c->conn->Join();
  }

  for (auto&& e : services_) {
    e->Stop();
  }

  // Wait for ongoing requests to complete.
  //
  // FIXME: Should we delay closing connections until all requests are finished?
  for (auto&& [_, c] : conns_temp) {
    c->handler->Stop();
  }
  for (auto&& [_, c] : conns_temp) {
    c->handler->Join();
    FLARE_CHECK_GT(alive_conns_.fetch_sub(1), 0);
  }
  for (auto&& e : services_) {
    e->Join();
  }
  while (alive_conns_) {
    this_fiber::SleepFor(10ms);
  }
  while (outstanding_jobs_) {
    this_fiber::SleepFor(10ms);
  }

  this_fiber::SleepFor(100ms);  // FIXME: Wait for timer to fully stop.
}


void Server::OnConnection(Handle fd, Endpoint peer) {
  FLARE_CHECK(!!fd);


  if (alive_conns_.fetch_add(1) >=
      options_.max_concurrent_connections) {
    FLARE_LOG_WARNING(
        "Too many connections. Connection from [{}] is rejected.",
        peer.ToString());
    alive_conns_.fetch_sub(1);
    return;
  }

  static const auto kSchedulingGroups = fiber::GetSchedulingGroupCount();
  static std::atomic<std::size_t> next_scheduling_group = 0;
  static std::atomic<std::size_t> conn_id = 0;

  auto scheduling_group = next_scheduling_group++ % kSchedulingGroups;

  // TODO(luobogao): Prevent TIME_WAIT here.

  FLARE_VLOG(10, "Accepted connection from [{}].", peer.ToString());

  // Initialize the socket.
  io::util::SetNonBlocking(fd.Get());
  io::util::SetCloseOnExec(fd.Get());
  io::util::SetTcpNoDelay(fd.Get());
  // `io::util::SetSendBufferSize` & `io::util::SetReceiveBufferSize`?

  auto icc = std::make_unique<ConnectionContext>();
  icc->scheduling_group_id = scheduling_group;
  icc->conn_id = ++conn_id;
  icc->remote_peer = peer;

  // Initialize the connection object.
  NativeStreamConnection::Options opts;
  opts.read_buffer_size = options_.maximum_packet_size;
  opts.handler = CreateNormalConnectionHandler(icc->conn_id, peer);
  

  icc->handler =
      static_cast<rpc::detail::ServerConnectionHandler*>(opts.handler.Get());
  icc->conn =
      std::make_shared<NativeStreamConnection>(std::move(fd), std::move(opts));

  // Retaining a reference here. If the connection is destroyed before we even
  // have a chance to call `StartHandshaking`, this reference avoid the risk of
  // use-after-free.
  auto desc = icc->conn;

  // Register the connection to the event loop.
  {
    std::scoped_lock lk(conns_lock_);
    conns_[icc->conn_id] = std::move(icc);
    // TODO(luobogao): Lock is held when calling `epoll_add`, what about
    // performance?
    GetGlobalEventLoop(scheduling_group) ->AttachDescriptor(std::dynamic_pointer_cast<Descriptor>(desc));
  }
  desc->StartHandshaking();
}

bool Server::OnNewCall() {
  if (FLARE_UNLIKELY(ongoing_calls_.fetch_add(1) >=
                     options_.max_concurrent_requests)) {
    FLARE_CHECK_GT(ongoing_calls_.fetch_sub(1), 0);
    FLARE_LOG_WARNING(
        "Overloaded. Maximum concurrent ongoing calls is capped to {}.",
        options_.max_concurrent_requests);
    return false;
  }
  return true;
}

void Server::OnCallCompletion() {
  FLARE_CHECK_GT(ongoing_calls_.fetch_sub(1), 0);
}

void Server::OnConnectionClosed(std::uint64_t id) {
  std::scoped_lock lk(conns_lock_);
  if (conns_.find(id) == conns_.end()) {
    FLARE_VLOG(10,
               "Connection #{} is not found. Perhaps it's removed by `Join()` "
               "or `OnConnectionCleanupTimer()`.",
               id);
    return;
  }
  FLARE_VLOG(10, "Closing connection from [{}].",
             conns_[id]->remote_peer.ToString());
  // We cannot destroy the connection until all request on it has been
  // processed.
  auto conn = std::move(conns_.at(id));
  conns_.erase(id);

  // Defer destruction until all requests has been processed.
  StartBackgroundJob([this, conn = std::move(conn)] {
    conn->conn->Stop();
    conn->conn->Join();
    conn->handler->Stop();
    // Note that calls on `StreamConnection` is no longer allowed after `Close`.
    // So wait until all pending requests are done before closing the
    // connection.
    conn->handler->Join();
    FLARE_CHECK_GT(alive_conns_.fetch_sub(1), 0);
  });
}

void Server::OnConnectionCleanupTimer() {
  std::vector<std::unique_ptr<ConnectionContext>> deleting;
  auto expire_threshold =
      ReadSteadyClock() - FLAGS_flare_rpc_server_connection_max_idle * 1s;
  {
    std::scoped_lock lk(conns_lock_);
    for (auto iter = conns_.begin(); iter != conns_.end();) {
      // TODO: implement `GetCoarseLastEventTimestamp`
      if (iter->second->handler->GetCoarseLastEventTimestamp() <
          expire_threshold) {
        deleting.push_back(std::move(iter->second));
        iter = conns_.erase(iter);
      } else {
        ++iter;
      }
    }
  }

  // Defer destruction until all requests has been processed.
  StartBackgroundJob([this, deleting = std::move(deleting)]() mutable {
    for (auto&& e : deleting) {
      e->conn->Stop();
    }
    for (auto&& e : deleting) {
      e->conn->Join();
    }
    for (auto&& e : deleting) {
      e->handler->Stop();
    }
    for (auto&& e : deleting) {
      e->handler->Join();
    }
    FLARE_CHECK_GE(
        alive_conns_.fetch_sub(deleting.size()),
        deleting.size());
  });
}

std::unique_ptr<rpc::detail::ServerConnectionHandler>
Server::CreateNormalConnectionHandler(std::uint64_t id, Endpoint peer) {
  auto ctx = std::make_unique<rpc::detail::NormalConnectionHandler::Context>();

  ctx->id = id;
  ctx->service_name = options_.service_name;
  ctx->local_peer = listening_on_;
  ctx->remote_peer = peer;
  ctx->max_request_queueing_delay = options_.max_request_queueing_delay;
  for (auto&& e : services_) {
    ctx->services.push_back(e.Get());
  }

  // Instantiate protocol.
  for (auto&& e : protocol_factories_) {
    ctx->protocols.push_back(e());
  }

  return std::make_unique<rpc::detail::NormalConnectionHandler>(this,
                                                                std::move(ctx));
}


void Server::StartBackgroundJob(Function<void()> cb) {
  outstanding_jobs_.fetch_add(1);

  flare::fiber::internal::StartFiberDetached([this, cb = std::move(cb)] {
    cb();
    FLARE_CHECK_GE(outstanding_jobs_.fetch_sub(1),
                   0);
  });
}

}  // namespace tinyRPC
