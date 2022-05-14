#ifndef _SRC_FIBER_DETAIL_FIBERENTITY_H_
#define _SRC_FIBER_DETAIL_FIBERENTITY_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>

#include "../../base/thread/SpinLock.h"
#include "../../base/ErasedPtr.h"
#include "../../base/Function.h"

namespace tinyRPC::fiber::detail{
    
class SchedulingGroup;
class ExitBarrier;

enum class FiberState{ READY, RUNNING, WAITING, DEAD };

// Fiber entities carry data and are indeed the underlying runnable object.
// Another considered name is `FiberData`.

// TODO: Do we need `enable_shared_from_this` ?
struct FiberEntity : public std::enable_shared_from_this<FiberEntity>{
public:
    FiberEntity();

    void* GetStackHighAddr() const noexcept;

    std::size_t GetStackSize() const noexcept { return stackSize_; }

    void Resume() noexcept;

    // Run this cb as resume_proc_ and then resume.
    void OnResume(UniqueFunction<void()>&& cb) noexcept;

    ErasedPtr* GetFiberLocalStorage(std::size_t index) noexcept;

public:    
    FiberState state_;

    // SchedulerLock protects when fiber in state transition.
    SpinLock schedulerLock_;

    // SchedulingGroup this fiber belongs to.
    SchedulingGroup* sg_;

    void* stackSaveBuffer_;
    std::size_t stackSize_;

    std::shared_ptr<ExitBarrier> exitBarrier_;

    std::unique_ptr<std::unordered_map<std::size_t, ErasedPtr>> fiberLocalStorage_; 

    // ResumeProc helps extra operation when scheduling.
    UniqueFunction<void()> resumeProc_;

    UniqueFunction<void()> startProc_;
};
    void SetUpMasterFiberEntity() noexcept;

    inline thread_local FiberEntity* masterFiber;

    inline thread_local FiberEntity* currentFiber; 

    inline void SetMasterFiberEntity(FiberEntity* master) noexcept { masterFiber = master; }

    inline void SetCurrentFiberEntity(FiberEntity* current) noexcept { currentFiber = current; }

    inline FiberEntity* GetMasterFiberEntity() noexcept { return masterFiber; } 

    inline FiberEntity* GetCurrentFiberEntity() noexcept { return currentFiber; }

    inline bool IsInFiberContext() noexcept { return GetCurrentFiberEntity() != nullptr; }

    inline void FreeFiberEntity(FiberEntity* fiber) noexcept;

    extern "C" void jump_context(void** self, void* to, void* context);

    extern "C" void* make_context(void* sp, std::size_t size, void (*start_proc)(void*));

    template <class F>
    void DestructiveRunCallback(F* cb) {
        (*cb)();
        *cb = nullptr;
    }

    template <class F>
    void DestructiveRunCallbackOpt(F* cb) {
        if (*cb) {
            (*cb)();
            *cb = nullptr;
        }
    }

    FiberEntity* CreateFiberEntity(SchedulingGroup* scheduling_group,
                                UniqueFunction<void()>&& startProc, 
                                std::shared_ptr<ExitBarrier>&& barrier) noexcept;

}

#endif