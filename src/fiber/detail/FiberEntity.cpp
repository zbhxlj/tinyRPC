#include <memory>

#include "../../include/glog/logging.h"

#include "FiberEntity.h"
#include "SchedulingGroup.h"
#include "Waitable.h"
#include "StackAllocator.h"

namespace tinyRPC::fiber::detail{

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

FiberEntity::FiberEntity() noexcept {}

void FiberEntity::OnResume(UniqueFunction<void()>&& cb) noexcept {
  auto caller = GetCurrentFiberEntity();
  CHECK(!resumeProc_) << "Execute `OnResume()` twice. (Before the previous one has finished)\n";
  CHECK_NE(caller, this) <<  "Calling `Resume()` on self is undefined.\n";

  resumeProc_ = std::move(cb);
  Resume();
}

ErasedPtr* FiberEntity::GetFiberLocalStorage(std::size_t index) noexcept {
    if(fiberLocalStorage_ == nullptr){
      fiberLocalStorage_ = std::make_unique<std::unordered_map<std::size_t, ErasedPtr>>();
    }
    return &(*fiberLocalStorage_)[index];
}

void SetUpMasterFiberEntity() noexcept {
  thread_local FiberEntity masterFiberImpl;

  masterFiberImpl.stackSaveBuffer_ = nullptr;
  masterFiberImpl.state_ = FiberState::RUNNING;
  masterFiberImpl.stackSize_ = 0;

  masterFiberImpl.sg_ = SchedulingGroup::Current();

  masterFiber = &masterFiberImpl;
  SetCurrentFiberEntity(masterFiber);
}

void* FiberEntity::GetStackHighAddr() const noexcept{
    return reinterpret_cast<char*>(const_cast<FiberEntity*>(this));
  }


FiberEntity* CreateFiberEntity(SchedulingGroup* scheduling_group,
                                UniqueFunction<void()>&& startProc, 
                                std::shared_ptr<ExitBarrier> barrier) noexcept {
  auto stack = CreateStack();
  auto stack_size = kStackSize - kPageSize;
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
  fiber->local_ = false;

  return fiber;
}

void FreeFiberEntity(FiberEntity* fiber) noexcept {
  fiber->~FiberEntity();

  auto p = reinterpret_cast<char*>(fiber) + sizeof(FiberEntity) - kStackSize;
  DestroyStack(p);
}

}