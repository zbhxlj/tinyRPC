#ifndef _TINYRPC_SRC_IO_DETAIL_READ_AT_MOST_H_
#define _TINYRPC_SRC_IO_DETAIL_READ_AT_MOST_H_

#include <string>

#include "../util/StreamIO.h"

namespace tinyRPC::io::detail {

enum class ReadStatus {
  // All bytes are read, the socket is left in `EAGAIN` state.
  Drained,

  // `max_bytes` are read.
  MaxBytesRead,

  // All (remaining) bytes are read, the socket is being closed by the remote
  // side.
  PeerClosing,

  // You're out of luck.
  Error
};

// Reads at most `max_bytes` into `to`.
//
// Returns whatever returned by `io->Read`.
//
// `to` is an empty string.
ReadStatus ReadAtMost(std::size_t max_bytes, AbstractStreamIo* io,
                      std::string& to, std::size_t* bytes_read);

}  // namespace tinyRPC::io::detail

#endif  
