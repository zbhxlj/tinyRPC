#include "WritingBufferList.h"

#include <fcntl.h>
#include <sys/stat.h>

#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "gtest/gtest.h"

#include "../../base/Latch.h"
#include "../util/Socket.h"
#include "../../testing/main.h"

using namespace std::literals;

namespace tinyRPC::io::detail {

TEST(WritingBufferList, Emptied) {
  WritingBufferList wbl;
  wbl.Append("123", 456);
  wbl.Append("2234", 567);
  int fd[2];  // read fd, write fd.
  PCHECK(pipe(fd) == 0);
  std::vector<std::uintptr_t> ctxs;
  bool emptied;
  bool short_write;
  auto io = std::make_unique<SystemStreamIo>(fd[1]);
  PCHECK(wbl.FlushTo(io.get(), 100, &ctxs, &emptied, &short_write) == 7);
  FLARE_LOG_INFO("8888\n");
  LOG(INFO) << "7777\n";
  printf("666666\n");
  ASSERT_EQ(2, ctxs.size());
  ASSERT_EQ(456, ctxs[0]);
  ASSERT_EQ(567, ctxs[1]);
  ASSERT_TRUE(emptied);
  ASSERT_FALSE(short_write);
  close(fd[0]);
  close(fd[1]);
}

TEST(WritingBufferList, Emptied2) {
  WritingBufferList wbl;
  wbl.Append("123", 456);
  wbl.Append("2234", 567);
  int fd[2];  // read fd, write fd.
  PCHECK(pipe(fd) == 0);
  std::vector<std::uintptr_t> ctxs;
  bool emptied;
  bool short_write;
  auto io = std::make_unique<SystemStreamIo>(fd[1]);
  PCHECK(wbl.FlushTo(io.get(), 7, &ctxs, &emptied, &short_write) == 7);
  ASSERT_EQ(2, ctxs.size());
  ASSERT_EQ(456, ctxs[0]);
  ASSERT_EQ(567, ctxs[1]);
  ASSERT_TRUE(emptied);
  ASSERT_FALSE(short_write);
  close(fd[0]);
  close(fd[1]);
}

TEST(WritingBufferList, PartialFlush) {
  WritingBufferList wbl;
  wbl.Append("123", 456);
  wbl.Append("2234", 567);
  int fd[2];  // read fd, write fd.
  PCHECK(pipe(fd) == 0);
  std::vector<std::uintptr_t> ctxs;
  bool emptied;
  bool short_write;
  auto io = std::make_unique<SystemStreamIo>(fd[1]);
  PCHECK(wbl.FlushTo(io.get(), 5, &ctxs, &emptied, &short_write) == 5);
  ASSERT_EQ(1, ctxs.size());
  ASSERT_EQ(456, ctxs[0]);
  ASSERT_FALSE(emptied);
  ASSERT_FALSE(short_write);
  close(fd[0]);
  close(fd[1]);
}

TEST(WritingBufferList, ShortWrite) {
  // @sa: http://man7.org/linux/man-pages/man7/pipe.7.html
  //
  // > In Linux versions before 2.6.11, the capacity of a pipe was the same as
  // > the system page size (e.g., 4096 bytes on i386).  Since Linux 2.6.11, the
  // > pipe capacity is 16 pages (i.e., 65,536 bytes in a system with a page
  // > size of 4096 bytes).  Since Linux 2.6.35, the default pipe capacity is 16
  // > pages, but the capacity can be queried and set using the fcntl(2)
  // > F_GETPIPE_SZ and F_SETPIPE_SZ operations.  See fcntl(2) for more
  // > information.
  //
  // Hopefully 64MB data could saturate system's buffer.
  constexpr auto kBufferSize = 64 * 1024 * 1024;

  WritingBufferList wbl;
  wbl.Append(std::string(kBufferSize, 'x'), 456);
  int fd[2];  // read fd, write fd.
  PCHECK(pipe(fd) == 0);
  util::SetNonBlocking(fd[0]);
  util::SetNonBlocking(fd[1]);
  std::vector<std::uintptr_t> ctxs;
  bool emptied;
  bool short_write;
  auto io = std::make_unique<SystemStreamIo>(fd[1]);
  auto rc = wbl.FlushTo(io.get(), kBufferSize, &ctxs, &emptied, &short_write);
  PCHECK(rc > 0);
  ASSERT_LT(rc, kBufferSize);
  ASSERT_EQ(0, ctxs.size());
  ASSERT_FALSE(emptied);
  ASSERT_TRUE(short_write);
  close(fd[0]);
  close(fd[1]);
}

TEST(DISABLED_WritingBufferList, Torture) {
  struct TestConfig {
    std::size_t loop;
    std::size_t buffer_size;
    std::size_t flush_limit;
  };
  constexpr auto N = 50;
  static const TestConfig configs[] = {
      {.loop = 50, .buffer_size = 10, .flush_limit = 1},
      {.loop = 500, .buffer_size = 5000, .flush_limit = 100000},
      {.loop = 5000,
       .buffer_size = 5000,
       .flush_limit = std::numeric_limits<std::size_t>::max()}};
  for (auto&& config : configs) {
    // FIXME:Due to implementation(std::string), it is likely to crash
    // if iteration too much for OOM.
    for (int i = 0; i != 2; ++i) {
      Handle fd(open("/dev/null", O_WRONLY));
      CHECK(!!fd);
      WritingBufferList wbl;
      Latch latch(1);
      std::atomic<std::size_t> written{};
      auto worker = [&] {
        latch.wait();
        for (int j = 0; j != config.loop; ++j) {
          auto nb = std::string(config.buffer_size, 'a');
          if (wbl.Append(std::move(nb), 100)) {
            while (true) {
              std::vector<std::uintptr_t> ctx;
              bool emptied, short_write;
              auto io = std::make_unique<SystemStreamIo>(fd.Get());
              auto rc = wbl.FlushTo(io.get(), config.flush_limit, &ctx,
                                    &emptied, &short_write);
              CHECK_GT(rc, 0);
              written += rc;
              CHECK(!short_write);
              if (emptied) {
                break;
              }
            }
          }
        }
      };
      std::thread ts[N];
      for (auto&& t : ts) {
        t = std::thread(worker);
      }
      latch.count_down();
      for (auto&& t : ts) {
        t.join();
      }
      std::this_thread::sleep_for(1ms);
      ASSERT_EQ(N * config.loop * config.buffer_size, written.load());
    }
  }
}

}  // namespace tinyRPC::io::detail

TINYRPC_TEST_MAIN
