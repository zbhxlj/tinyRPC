#ifndef _SRC_BASE_BASE_H_
#define _SRC_BASE_BASE_H_

#include <pthread.h>
#include <cstdint>
namespace tinyRPC{
    void SetCurrentThreadName(const char* name);
}

#endif