#ifndef _TINYRPC_SRC_IO_UTIL_STREAM_IO_H_
#define _TINYRPC_SRC_IO_UTIL_STREAM_IO_H_

#include <sys/uio.h>

#include <cstddef>
#include <string>

#include "../detail/EintrSafe.h"

namespace tinyRPC {

class AbstractStreamIo {
 public:
  virtual ~AbstractStreamIo() = default;

  enum class HandshakingStatus { Success, WannaRead, WannaWrite, Error };

  virtual HandshakingStatus Handshake() = 0;

  virtual ssize_t Read(std::string& buf) = 0;

  // TODO: whether `const` ?
  virtual ssize_t Write(std::string& buf) = 0;
};

class SystemStreamIo : public AbstractStreamIo {
 public:
  explicit SystemStreamIo(int fd) : fd_(fd) {}

  HandshakingStatus Handshake() override { return HandshakingStatus::Success; }

  ssize_t Read(std::string& buf) override {
    return io::detail::EIntrSafeRead(fd_, const_cast<char*>(buf.c_str()), buf.capacity());
  }

  ssize_t Write(std::string& buf) override {
    return io::detail::EIntrSafeWrite(fd_, const_cast<char*>(buf.c_str()), buf.capacity());
  }

  int GetFd() const noexcept { return fd_; }

 private:
  int fd_;
};

}  // namespace tinyRPC

#endif  
