#ifndef _SRC_FIBER_DETAIL_STACK_ALLOCATOR_H_
#define _SRC_FIBER_DETAIL_STACK_ALLOCATOR_H_

namespace tinyRPC::fiber::detail{

void* CreateStack();

void DestroyStack(void* ptr);

} // namespace tinyRPC::fiber::detail

#endif