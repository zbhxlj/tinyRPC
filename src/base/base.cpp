#include <cstdlib>

#include "base.h"

namespace tinyRPC{
    void SetCurrentThreadName(const char* name){
        auto self = pthread_self();
        pthread_setname_np(self, name);
    }

    std::uint64_t Random(std::uint64_t num){
        std::srand(time(NULL));
        return rand() % num;
    }
}