#include "base.h"

namespace tinyRPC{
    void SetCurrentThreadName(const char* name){
        auto self = pthread_self();
        pthread_setname_np(self, name);
    }
}