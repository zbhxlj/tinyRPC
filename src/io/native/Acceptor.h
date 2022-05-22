#ifndef _SRC_IO_NATIVE_ACCEPTOR_H_
#define _SRC_IO_NATIVE_ACCEPTOR_H_

#include "../../base/Function.h"
#include "../../base/Endpoint.h"
#include "../../base/Handle.h"

#include "../Acceptor.h"
#include "../Descriptor.h"

namespace tinyRPC {

// `NativeAcceptor` listens on a TCP port for incoming connections.
class NativeAcceptor final : public Descriptor, public Acceptor {
 public:
  struct Options {
    // Called when new connection is accepted.
    //
    // The handler is responsible for setting `FD_CLOEXEC` / `O_NONBLOCK` and
    // whatever it sees need.
    //
    // CAVEAT: Due to technical limitations, it's likely that
    // `connection_handler` is not called in a balanced fashion if same `fd`
    // (see above) is associated with `NativeAcceptor`s bound with different
    // `EventLoop`. You may have to write your own logic to balance work loads.
    // This imbalance is probably significant if you're trying to use multiple
    // `NativeAcceptor`s to balance workloads between several program-defined
    // worker domains. A possible choice would be dispatch the requests into
    // each NUMA domain in a round-robin fashion in `connection_handler`, for
    // example.
    UniqueFunction<void(Handle fd, Endpoint peer)> connection_handler;
  };

  // O_NONBLOCK must be set on `fd`. Ownership is taken.
  //
  // The caller is responsible for bind / listen / etc.. `NativeAcceptor` is
  // only responsible for "accept"ing connections from `fd`.
  explicit NativeAcceptor(Handle fd, Options options);

  void Stop() override;
  void Join() override;

 private:
  EventAction OnReadable() override;
  EventAction OnWritable() override;  // Abort on call.
  void OnError(int err) override;
  void OnCleanup(CleanupReason reason) override;

 private:
  Options options_;
};

}  // namespace tinyRPC

#endif 
