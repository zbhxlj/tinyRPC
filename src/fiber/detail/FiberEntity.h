#ifndef _SRC_FIBER_DETAIL_FIBERENTITY_H_
#define _SRC_FIBER_DETAIL_FIBERENTITY_H_

#include <cstddef>
#include <functional>

namespace tinyRPC::fiber::detail{
    
class SchedulingGroup;

enum class FiberState{ READY, RUNNING, WAITING, DEAD };

class FiberEntity{

private:    
    FiberState state_;

    // SchedulerLock protects when fiber in state transition.
    SpinLock schedulerLock_;

    // SchedulingGroup this fiber belongs to.
    SchedulingGroup* sg_;

    void* stackSaveBuffer_;
    std::size_t stackSize_;

    // TODO: Is std::shared_ptr enough to substitue RefPtr?
    RefPtr<ExitBarrier> exitBarrier_;

    // TODO: How to implement fiber local storage.
    
    // ResumeProc helps extra operation when scheduling.
    std::function<void()> resumeProc_;

    std::function<void()> startProc_;
};

}

#endif