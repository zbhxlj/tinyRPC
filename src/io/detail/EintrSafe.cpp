#include "EintrSafe.h"

namespace tinyRPC::io::detail {

int EIntrSafeAccept(int sockfd, sockaddr* addr, socklen_t* addrlen) {
  return EIntrSafeCall([&] { return accept(sockfd, addr, addrlen); });
}

int EIntrSafeEpollWait(int epfd, epoll_event* events, int maxevents,
                       int timeout) {
  return EIntrSafeCall(
      [&] { return epoll_wait(epfd, events, maxevents, timeout); });
}

ssize_t EIntrSafeRecvFrom(int sockfd, void* buf, size_t len, int flags,
                          sockaddr* src_addr, socklen_t* addrlen) {
  return EIntrSafeCall(
      [&] { return recvfrom(sockfd, buf, len, flags, src_addr, addrlen); });
}

ssize_t EIntrSafeSendTo(int sockfd, const void* buf, size_t len, int flags,
                        const sockaddr* dest_addr, socklen_t addrlen) {
  return EIntrSafeCall(
      [&] { return sendto(sockfd, buf, len, flags, dest_addr, addrlen); });
}

ssize_t EIntrSafeSendMsg(int sockfd, const msghdr* msg, int flags) {
  return EIntrSafeCall([&] { return sendmsg(sockfd, msg, flags); });
}

}  // namespace tinyRPC::io::detail
