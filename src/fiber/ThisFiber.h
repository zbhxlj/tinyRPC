#ifndef _SRC_FIBER_THIS_FIBER_H_    
#define _SRC_FIBER_THIS_FIBER_H_

#include <chrono>

namespace tinyRPC::this_fiber{

void Yield();

void SleepUntil(std::chrono::steady_clock::time_point expires_at);

void SleepFor(std::chrono::nanoseconds expires_in);

} // namespace tinyRPC::this_fiber

#endif