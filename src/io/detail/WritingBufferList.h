#ifndef _TINYRPC_SRC_IO_DETAIL_WRITING_BUFFER_LIST_H_
#define _TINYRPC_SRC_IO_DETAIL_WRITING_BUFFER_LIST_H_

#include <vector>
#include <atomic>
#include <string>
#include <list>

#include "../../include/gtest/gtest_prod.h"

#include "../../fiber/Mutex.h"
#include "../util/StreamIO.h"

namespace tinyRPC::io::detail {

// An MPSC writing buffer queue.
class WritingBufferList {
 public:
  WritingBufferList();
  ~WritingBufferList();

  ssize_t FlushTo(tinyRPC::AbstractStreamIo* io, std::size_t max_bytes,
                  std::vector<std::uintptr_t>* flushed_ctxs, bool* emptied,
                  bool* short_write);
  bool Append(std::string buffer, std::uintptr_t ctx);

 private:
//   FRIEND_TEST(WritingBufferList, Torture);

  struct Node {
    Node(std::string buf, std::uintptr_t ctx): buffer(buf), ctx(ctx){}
    std::string buffer;
    std::uintptr_t ctx;
  };

  std::list<Node*> buffers_;
  // TODO: to pass test, use std::mutex temporarily.
  std::mutex lock_;
  // tinyRPC::fiber::Mutex lock_;
};

}  // namespace tinyRPC::io::detail

#endif  
