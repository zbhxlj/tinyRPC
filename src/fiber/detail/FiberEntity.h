#ifndef _SRC_FIBER_DETAIL_FIBERENTITY_H_
#define _SRC_FIBER_DETAIL_FIBERENTITY_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>

#include "../../base/thread/SpinLock.h"
#include "../../base/ErasedPtr.h"

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
    void OnResume(std::function<void()>&& cb) noexcept;

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
    std::function<void()> resumeProc_;

    std::function<void()> startProc_;
};
    void SetUpMasterFiberEntity() noexcept;

    thread_local FiberEntity* masterFiber;

    thread_local FiberEntity* currentFiber; 

    void SetMasterFiberEntity(FiberEntity* master) noexcept { masterFiber = master; }

    void SetCurrentFiberEntity(FiberEntity* current) noexcept { currentFiber = current; }

    FiberEntity* GetMasterFiberEntity() noexcept { return masterFiber; } 

    FiberEntity* GetCurrentFiberEntity() noexcept { return currentFiber; }

    bool IsInFiberContext() noexcept { return GetCurrentFiber() != nullptr; }

    void FreeFiberEntity(FiberEntity* fiber) noexcept;

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

}

#endif