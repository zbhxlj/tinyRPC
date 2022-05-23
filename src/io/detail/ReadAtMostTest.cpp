#include "ReadAtMost.h"

#include <fcntl.h>
#include <unistd.h>

#include <limits>
#include <memory>
#include <string>
#include <thread>

#include "../../include/gtest/gtest.h"

#include "../../base/Random.h"
#include "../../base/Logging.h"
#include "../util/Socket.h"

using namespace std::literals;

namespace tinyRPC::io::detail {

class ReadAtMostTest : public ::testing::Test {
 public:
  void SetUp() override {
    PCHECK(pipe(fd_) == 0);
    PCHECK(write(fd_[1], "1234567", 7) == 7);
    util::SetNonBlocking(fd_[0]);
    util::SetNonBlocking(fd_[1]);
    io_ = std::make_unique<SystemStreamIo>(fd_[0]);
    buffer_.clear();
  }
  void TearDown() override {
    // Failure is ignored. If the testcase itself closed the socket, call(s)
    // below can fail.
    close(fd_[0]);
    close(fd_[1]);
  }

 protected:
  int fd_[2];  // read fd, write fd.
  std::unique_ptr<SystemStreamIo> io_;
  std::string buffer_;
  std::size_t bytes_read_;
};

TEST_F(ReadAtMostTest, Drained) {
  ASSERT_EQ(ReadStatus::Drained,
            ReadAtMost(8, io_.get(), buffer_, &bytes_read_));
  EXPECT_EQ(std::string("1234567"), buffer_);
  EXPECT_EQ(7, bytes_read_);
}

TEST_F(ReadAtMostTest, Drained2) {
  buffer_ = "0000";
  ASSERT_EQ(ReadStatus::Drained,
            ReadAtMost(8, io_.get(), buffer_, &bytes_read_));
  EXPECT_EQ(std::string("00001234567"), buffer_);
  EXPECT_EQ(7, bytes_read_);
}

TEST_F(ReadAtMostTest, MaxBytesRead) {
  ASSERT_EQ(ReadStatus::MaxBytesRead,
            ReadAtMost(7, io_.get(), buffer_, &bytes_read_));
  EXPECT_EQ(std::string("1234567"), buffer_);
  EXPECT_EQ(7, bytes_read_);
}

TEST_F(ReadAtMostTest, MaxBytesRead2) {
  ASSERT_EQ(ReadStatus::MaxBytesRead,
            ReadAtMost(5, io_.get(), buffer_, &bytes_read_));
  EXPECT_EQ(std::string("12345"), buffer_);
  EXPECT_EQ(5, bytes_read_);
}

TEST_F(ReadAtMostTest, PeerClosing) {
  FLARE_PCHECK(close(fd_[1]) == 0);
  ASSERT_EQ(ReadStatus::Drained,
            ReadAtMost(8, io_.get(), buffer_, &bytes_read_));
  EXPECT_EQ(7, bytes_read_);
  // This is weird. The first call always succeeds even if it can tell the
  // remote side has closed the socket, yet we still need to issue another call
  // to `read` to see the situation.
  ASSERT_EQ(ReadStatus::PeerClosing,
            ReadAtMost(1, io_.get(), buffer_, &bytes_read_));
  EXPECT_EQ(0, bytes_read_);
  EXPECT_EQ(std::string("1234567"), buffer_);
}

TEST(ReadAtMost, LargeChunk) {
  // @sa: https://man7.org/linux/man-pages/man2/fcntl.2.html
  //
  // > Note that because of the way the pages of the pipe buffer are employed
  // > when data is written to the pipe, the number of bytes that can be written
  // > may be less than the nominal size, depending on the size of the writes.
  constexpr auto kMaxBytes = 1048576;  // @sa: `/proc/sys/fs/pipe-max-size`

  int fd[2];  // read fd, write fd.
  FLARE_PCHECK(pipe(fd) == 0);
  FLARE_PCHECK(fcntl(fd[0], F_SETPIPE_SZ, kMaxBytes) == kMaxBytes);
  util::SetNonBlocking(fd[0]);
  util::SetNonBlocking(fd[1]);
  auto io = std::make_unique<SystemStreamIo>(fd[0]);

  std::srand(time(NULL));
  std::string source;
  for (int i = 0; i != kMaxBytes; ++i) {
    source.push_back(rand() % 128);
  }

  for (int i = 1; i < kMaxBytes; i = i * 3 / 2 + 17) {
    FLARE_LOG_INFO("Testing chunk of size {}.", i);

    FLARE_PCHECK(write(fd[1], source.data(), i) == i);

    std::string buffer;
    std::size_t bytes_read;

    if (rand() % 2 == 0) {
      ASSERT_EQ(ReadStatus::Drained,
                ReadAtMost(i + 1, io.get(), buffer, &bytes_read));
    } else {
      ASSERT_EQ(ReadStatus::MaxBytesRead,
                ReadAtMost(i, io.get(), buffer, &bytes_read));
    }
    EXPECT_EQ(i, bytes_read);
    // Not using `EXPECT_EQ` as diagnostics on error is potentially large, so we
    // want to bail out on error ASAP.
    ASSERT_EQ(buffer, source.substr(0, i));
  }
}

}  // namespace tinyRPC::io::detail

