#ifndef _TINYRPC_IO_DETAIL_EINTR_SAFE_H_
#define _TINYRPC_IO_DETAIL_EINTR_SAFE_H_

#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include <utility>


namespace tinyRPC::io::detail {

template <class F>
auto EIntrSafeCallSlow(F&& f) {
  while (true) {
    auto rc = std::forward<F>(f)();
    if (rc >= 0 || errno != EINTR) {
      return rc;
    }
  }
}

template <class F>
inline auto EIntrSafeCall(F&& f) {
  auto rc = std::forward<F>(f)();
  if (rc >= 0 || errno != EINTR) {
    return rc;
  }

  return EIntrSafeCallSlow(std::forward<F>(f));
}

inline ssize_t EIntrSafeRead(int fd, void* buf, size_t count) {
  return EIntrSafeCall([&] { return read(fd, buf, count); });
}

inline ssize_t EIntrSafeWrite(int fd, const void* buf, size_t count) {
  return EIntrSafeCall([&] { return write(fd, buf, count); });
}

inline ssize_t EIntrSafeReadV(int fd, const iovec* iov, int iovcnt) {
  return EIntrSafeCall([&] { return readv(fd, iov, iovcnt); });
}

inline ssize_t EIntrSafeWriteV(int fd, const iovec* iov, int iovcnt) {
  return EIntrSafeCall([&] { return writev(fd, iov, iovcnt); });
}

int EIntrSafeAccept(int sockfd, sockaddr* addr, socklen_t* addrlen);
int EIntrSafeEpollWait(int epfd, epoll_event* events, int maxevents,
                       int timeout);
ssize_t EIntrSafeRecvFrom(int sockfd, void* buf, size_t len, int flags,
                          sockaddr* src_addr, socklen_t* addrlen);
ssize_t EIntrSafeSendTo(int sockfd, const void* buf, size_t len, int flags,
                        const sockaddr* dest_addr, socklen_t addrlen);
ssize_t EIntrSafeSendMsg(int sockfd, const struct msghdr* msg, int flags);

}  // namespace tinyRPC::io::detail

#endif  
