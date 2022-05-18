#ifndef _SRC_BASE_CHRONO_H_
#define _SRC_BASE_CHRONO_H_

#include <chrono>

namespace tinyRPC{

using namespace std::chrono_literals;

inline auto ReadSteadyClock() noexcept{
    return std::chrono::steady_clock::now();
}

inline auto ReadSystemClock() noexcept{
    return std::chrono::system_clock::now();
}

} // namespace tinyRPC

#endif