#include <unistd.h>
#include <sys/mman.h>

#include "../../../include/glog/logging.h"
#include "StackAllocator.h"

namespace tinyRPC::fiber::detail{
    
constexpr uint64_t kPageSize = 4 * 1024;

void* CreateStack(){
    auto p = mmap(nullptr, kStackSize, PROT_READ | PROT_WRITE, 
        MAP_ANONYMOUS | MAP_PRIVATE | MAP_STACK, 0, 0);
    CHECK(p) << "Mmap failed : out-of-memory error\n";
    CHECK_EQ(reinterpret_cast<std::uint64_t>(p) % kPageSize, 0) 
            << "Mmap error : addr not aligned with page size\n";
    CHECK_EQ(mprotect(p, kPageSize, PROT_NONE), 0) 
            << "Mprotect error : out-of-memory error\n";   

    auto stack = reinterpret_cast<char*>(p);
    stack += kPageSize;

    return reinterpret_cast<void*>(stack);
}


void DestroyStack(void* ptr){
    CHECK_EQ(munmap(ptr, kStackSize), 0) << "Munmap error\n";
}

} // namespace tinyRPC::fiber::detail