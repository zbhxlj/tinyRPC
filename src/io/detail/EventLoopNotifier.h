#ifndef _TINYRPC_IO_DETAIL_EVENT_LOOP_NOTIFIER_H_
#define _TINYRPC_IO_DETAIL_EVENT_LOOP_NOTIFIER_H_

#include "../../base/Handle.h"

namespace tinyRPC::io::detail {

// Wrapper of eventfd to wake up `EventLoop`.
class EventLoopNotifier final {
 public:
  EventLoopNotifier();

  int fd() const noexcept;

  // Wake up the event loop.
  void Notify() noexcept;

  // Once woken up, it's the event loop's responsibility to call this to drain
  // any pending events signaled by `Notify`.
  void Reset() noexcept;

 private:
  tinyRPC::Handle fd_;
};

}  // namespace tinyRPC::io::detail

#endif  