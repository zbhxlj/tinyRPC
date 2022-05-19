#include <fcntl.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "../../include/glog/logging.h"

#include "EventLoopNotifier.h"
#include "EintrSafe.h"

namespace tinyRPC::io::detail {

namespace {

Handle CreateEvent() {
  Handle fd(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK));
  CHECK(fd) << "Create eventfd failed.";
  return fd;
}

}  // namespace

EventLoopNotifier::EventLoopNotifier() : fd_(CreateEvent()) {}

int EventLoopNotifier::fd() const noexcept { return fd_.Get(); }

void EventLoopNotifier::Notify() noexcept {
  std::uint64_t v = 1;
  PCHECK(io::detail::EIntrSafeWrite(fd(), &v, sizeof(v)) == sizeof(v));
}

void EventLoopNotifier::Reset() noexcept {
  std::uint64_t v;
  int rc;

  // Drain off.
  do {
    rc = io::detail::EIntrSafeRead(fd(), &v, sizeof(v));
    PCHECK(rc >= 0 || errno == EAGAIN || errno == EWOULDBLOCK);
  } while (rc > 0);
}

}  // namespace tinyRPC::io::detail
