#ifndef _SRC_BASE_INDEX_ALLOC_H_
#define _SRC_BASE_INDEX_ALLOC_H_

#include <mutex>
#include <vector>

#include "Singleton.h"

namespace tinyRPC{

class IndexAlloc : public Singleton<IndexAlloc>{
public:
  void Free(std::size_t index);

  std::size_t Next();

private:
  friend class Singleton<IndexAlloc>;
  IndexAlloc() = default;
  ~IndexAlloc() = default;

  std::mutex lock_;
  std::size_t current_{};
  std::vector<std::size_t> recycled_;
};

} // namespace tinyRPC

#endif