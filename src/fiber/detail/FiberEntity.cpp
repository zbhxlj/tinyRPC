#include <memory>
#include "glog/logging.h"
#include "gflags/gflags.h"

#include "FiberEntity.h"

namespace tinyRPC::fiber::detail{

DEFINE_uint32(FiberStackSize, 128 * 1024, "Fiber Stack Size");

void FiberEntity::Resume() noexcept {
  auto caller = GetCurrentFiberEntity();
  CHECK_NE(caller, this) << "Calling `Resume()` on self is undefined." << std::endl;

  jump_context(&caller->stackSaveBuffer_, stackSaveBuffer_, this);

  SetCurrentFiberEntity(caller);  // The caller has back.

  // Check for pending `OnResume`.
  DestructiveRunCallbackOpt(&caller->resumeProc_);
}

static void FiberProc(void* context){
    auto self = static_cast<FiberEntity*>(context);

    SetCurrentFiberEntity(self);  
    self->state_ = FiberState::RUNNING;

    DestructiveRunCallbackOpt(&self->resumeProc_);
    DestructiveRunCallback(&self->startProc_);

    CHECK_EQ(self, GetCurrentFiberEntity());

    // No one will join us.
    if (!self->exitBarrier_) {
        self->state_ = FiberState::DEAD;

        // Free self.
        GetMasterFiberEntity()->OnResume([self] { FreeFiberEntity(self); });
    } else {
        // Grab lock first to avoid block when running on master fiber.
        // TODO: implement `ExitBarrier`
        auto ebl = self->exitBarrier_->GrabLock();

        self->state_ = FiberState::DEAD;

        GetMasterFiberEntity()->OnResume([self, lk = std::move(ebl)]() mutable {
            auto eb = std::move(self->exitBarrier_);

            // Maybe reschedule when count_down(), so free it early here.
            FreeFiberEntity(self);  // Good-bye.

            // Were anyone waiting on us, wake them up now.
            eb->UnsafeCountDown(std::move(lk));
        });
    }
    CHECK(0);  // Can't be here.
}

FiberEntity::FiberEntity() {}

void FiberEntity::OnResume(std::function<void()>&& cb) noexcept {
  auto caller = GetCurrentFiberEntity();
  CHECK(!resumeProc_) << "Execute `OnResume()` twice. (Before the previous one has finished)\n";
  CHECK_NE(caller, this) <<  "Calling `Resume()` on self is undefined.\n";

  resumeProc_ = std::move(cb);
  Resume();
}

ErasedPtr* FiberEntity::GetFiberLocalStorage(std::size_t index) noexcept {
    CHECK_NE(fiberLocalStorage_->end(), fiberLocalStorage_.find(index));
    return (*fiberLocalStorage_)[index];
}

void SetUpMasterFiberEntity() noexcept {
  thread_local FiberEntity masterFiberImpl;

  masterFiberImpl.stackSaveBuffer_ = nullptr;
  masterFiberImpl.state_ = FiberState::RUNNING;
  masterFiberImpl.stackSize_ = 0;

  // TODO: implement scheduling group. 
  masterFiberImpl.sg_ = SchedulingGroup::Current();

  masterFiber = &masterFiberImpl;
  SetCurrentFiberEntity(masterFiber);
}


FiberEntity* CreateFiberEntity(SchedulingGroup* scheduling_group,
                                std::function<void()>&& startProc, 
                                std::shared_ptr<ExitBarrier_>&& barrier) noexcept {
  // TODO: implement `UserStack`.
  auto stack = CreateUserStack();
  auto stack_size = FLAGS_FiberStackSize;
  auto bottom = reinterpret_cast<char*>(stack) + stack_size;
  // `FiberEntity` is stored at the stack bottom.
  auto ptr = bottom - sizeof(FiberEntity);

  FiberEntity* fiber = new (ptr) FiberEntity;  // A new life has born.

  fiber->stackSize_ = stack_size - sizeof(FiberEntity);
  fiber->stackSaveBuffer_ = make_context(fiber->GetStackHighAddr(), fiber->GetStackSize(), FiberProc);
  fiber->sg_ = scheduling_group;
  fiber->state_ = FiberState::READY;

  fiber->startProc_ = std::move(startProc);
  fiber->exitBarrier_ = std::move(barrier);

  return fiber;
}

void FreeFiberEntity(FiberEntity* fiber) noexcept {
  fiber->~FiberEntity();

  auto p = reinterpret_cast<char*>(fiber) + sizeof(FiberEntity) - FLAGS_FiberStackSize;
  // TODO: implement `UserStack`.
  FreeUserStack(p);
}

}