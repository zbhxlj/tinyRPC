#include "../../include/glog/logging.h"

#include "ReadAtMost.h"

namespace tinyRPC::io::detail{

ssize_t ReadAtMostPartial(std::size_t max_bytes, AbstractStreamIo* io,
                          std::string& to, bool* short_read) {
  char buf[max_bytes];
  memset(buf, 0, sizeof(buf));

  auto readBytes = io->Read(buf, max_bytes);
  if (readBytes <= 0) {
    return readBytes;
  }

  CHECK_LE(readBytes, max_bytes);
  *short_read = readBytes != max_bytes;
  
  if(readBytes){
      to.append(buf, readBytes);
  }

  return readBytes;
}

ReadStatus ReadAtMost(std::size_t max_bytes, AbstractStreamIo* io,
                      std::string& to, std::size_t* bytes_read) {
  auto bytes_left = max_bytes;
  *bytes_read = 0;
  while (bytes_left) {
    bool short_read = false;  
    auto bytes_to_read = bytes_left;
    auto read = ReadAtMostPartial(bytes_to_read, io, to, &short_read);
    if (read == 0) {  
      return ReadStatus::PeerClosing;
    }
    if (read < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return ReadStatus::Drained;
      } else {
        return ReadStatus::Error;
      }
    }
    CHECK_LE(read, bytes_to_read);

    // Let's update the statistics.
    *bytes_read += read;
    bytes_left -= read;

    if (short_read) {
      CHECK_LT(read, bytes_to_read);
      return ReadStatus::Drained;
    }
  }
  CHECK_EQ(*bytes_read, max_bytes);
  return ReadStatus::MaxBytesRead;
}

} // namespace tinyRPC::io::detail