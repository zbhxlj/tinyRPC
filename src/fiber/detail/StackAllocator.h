#ifndef _SRC_FIBER_DETAIL_STACK_ALLOCATOR_H_
#define _SRC_FIBER_DETAIL_STACK_ALLOCATOR_H_

#include <cstdint>

namespace tinyRPC::fiber::detail{

constexpr uint32_t kStackSize = 1024 * 128;

void* CreateStack();

void DestroyStack(void* ptr);

} // namespace tinyRPC::fiber::detail

#endif