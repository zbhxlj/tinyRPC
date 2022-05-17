#ifndef _SRC_FIBER_RUNTIME_H_
#define _SRC_FIBER_RUNTIME_H_

#include <cstdint>

#include "detail/SchedulingGroup.h"

namespace tinyRPC::fiber{

using namespace detail;
std::size_t GetCurrentSchedulingGroupIndex();
void StartRuntime();
void TerminateRuntime();
std::size_t GetSchedulingGroupCount();
std::size_t GetSchedulingGroupSize();

SchedulingGroup* GetSchedulingGroup(std::size_t index);

SchedulingGroup* NearestSchedulingGroupSlow(SchedulingGroup** cache);
inline SchedulingGroup* NearestSchedulingGroup() {
  thread_local SchedulingGroup* nearest{};
  if (nearest) {
    return nearest;
  }
  return NearestSchedulingGroupSlow(&nearest);
}

std::ptrdiff_t NearestSchedulingGroupIndex();

} // namespace tinyRPC::fiber

#endif