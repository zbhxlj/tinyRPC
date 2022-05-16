#include "FiberWorker.h"
#include <cstddef>
#include "FiberEntity.h"
#include "SchedulingGroup.h"

namespace tinyRPC::fiber::detail{

FiberWorker::FiberWorker(SchedulingGroup* sg, std::size_t worker_index)
    : sg_(sg), workerIndex_(worker_index) {}

void FiberWorker::AddForeignSchedulingGroup(SchedulingGroup* sg,
                                            std::uint64_t steal_every_n) {
  std::srand(time(NULL));
  victims_.push({.sg = sg,
                 .steal_every_n = steal_every_n,
                 .next_steal = std::rand() % steal_every_n });
}

void FiberWorker::Start() {
  worker_ = std::thread([this] {
    WorkerProc();
  });
}

void FiberWorker::Join() { worker_.join(); }

void FiberWorker::WorkerProc() {
  sg_->EnterGroup(workerIndex_);

  while (true) {
    auto fiber = sg_->AcquireFiber();

    if (!fiber) {
      if (!fiber) {
        fiber = StealFiber();
        CHECK_NE(fiber, SchedulingGroup::kSchedulingGroupShuttingDown);
        if (!fiber) {
          fiber = sg_->WaitForFiber(); 
          CHECK_NE(fiber, static_cast<FiberEntity*>(nullptr));
        }
    }
    }

    if (fiber == SchedulingGroup::kSchedulingGroupShuttingDown) {
      break;
    }

    fiber->Resume();

  }
  CHECK_EQ(GetCurrentFiberEntity(), GetMasterFiberEntity());
  sg_->LeaveGroup();
}

FiberEntity* FiberWorker::StealFiber() {
  if (victims_.empty()) {
    return nullptr;
  }

  ++stealVecClock_;
  while (victims_.top().next_steal <= stealVecClock_) {
    auto&& top = victims_.top();
    if (auto rc = top.sg->RemoteAcquireFiber()) {
      // We don't pop the top in this case, since it's not empty, maybe the next
      // time we try to steal, there are still something for us.
      return rc;
    }
    victims_.push({.sg = top.sg,
                   .steal_every_n = top.steal_every_n,
                   .next_steal = top.next_steal + top.steal_every_n});
    victims_.pop();
  }
  return nullptr;
}

} // namespace tinyRPC::fiber::detail