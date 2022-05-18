#ifndef _SRC_FIBER_TIMER_H_
#define _SRC_FIBER_TIMER_H_

#include <chrono>

#include "detail/TimerWorker.h"
#include "../base/Function.h"

namespace tinyRPC::fiber{

[[nodiscard]] detail::TimerPtr SetTimer(std::chrono::steady_clock::time_point at,
                                     UniqueFunction<void()>&& cb);
detail::TimerPtr SetTimer(std::chrono::steady_clock::time_point at,
                                     UniqueFunction<void(detail::TimerPtr& )>&& cb);

[[nodiscard]] detail::TimerPtr SetTimer(std::chrono::steady_clock::time_point at,
                                     std::chrono::nanoseconds interval,
                                     UniqueFunction<void()>&& cb);

detail::TimerPtr SetTimer(std::chrono::steady_clock::time_point at,
                                     std::chrono::nanoseconds interval,
                                     UniqueFunction<void(detail::TimerPtr&)>&& cb);

[[nodiscard]] detail::TimerPtr SetTimer(std::chrono::nanoseconds interval,
                                     UniqueFunction<void()>&& cb);    

detail::TimerPtr SetTimer(std::chrono::nanoseconds interval,
                                     UniqueFunction<void(detail::TimerPtr&)>&& cb);             

void KillTimer(detail::TimerPtr timer_id);

// For internal use, mind that we don't start a fiber
// to run timer callback.
namespace internal{

[[nodiscard]] detail::TimerPtr CreateTimer(
    std::chrono::steady_clock::time_point at,
    UniqueFunction<void(detail::TimerPtr&)>&& cb);
[[nodiscard]] detail::TimerPtr CreateTimer(
    std::chrono::steady_clock::time_point at, std::chrono::nanoseconds interval,
    UniqueFunction<void(detail::TimerPtr&)>&& cb);

void EnableTimer(detail::TimerPtr& timer_id);

void KillTimer(detail::TimerPtr& timer_id);

} // namespace tinyRPC::fiber::internal

} // namespace tinyRPC::fiber

#endif