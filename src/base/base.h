#ifndef _SRC_BASE_BASE_H_
#define _SRC_BASE_BASE_H_

#include <pthread.h>
#include <cstdint>
namespace tinyRPC{
    void SetCurrentThreadName(const char* name);
    std::uint64_t Random(std::uint64_t val);
}

#endif