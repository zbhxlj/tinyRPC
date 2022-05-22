#ifndef _SRC_IO_UTIL_SOCKET_H_
#define _SRC_IO_UTIL_SOCKET_H_

#include "../../base/Handle.h"
#include "../../base/Endpoint.h"

namespace tinyRPC::io::util {

// Returns a invalid handle (`(!rc == true)` holds.) on failure.

// `backlog` is capped by `net.core.somaxconn`: https://serverfault.com/q/518862
//
// If you're not able to accept connections quick enough, you're likely to lose
// them or have other troubles with accepting them.
Handle CreateListener(const Endpoint& addr, int backlog);

// For client side's use.
Handle CreateStreamSocket(sa_family_t family);
Handle CreateDatagramSocket(sa_family_t family);

bool StartConnect(int fd, const Endpoint& addr);

void SetNonBlocking(int fd);
void SetCloseOnExec(int fd);
void SetTcpNoDelay(int fd);

// Internally the kernel will double `size`.
//
// @sa: https://www.man7.org/linux/man-pages/man7/socket.7.html
void SetSendBufferSize(int fd, int size);
void SetReceiveBufferSize(int fd, int size);

int GetSocketError(int fd);

}  // namespace tinyRPC::io::util

#endif  
