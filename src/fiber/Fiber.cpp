#include "../base/Random.h"

#include "Fiber.h"
#include "detail/FiberEntity.h"
#include "detail/SchedulingGroup.h"
#include "Runtime.h"


// TODO: runtime.h
namespace tinyRPC::fiber{

using detail::SchedulingGroup;

namespace {

[[maybe_unused]] SchedulingGroup* GetSchedulingGroupByID(std::size_t id) {
  if (id == Fiber::kNearestSchedulingGroup) {
    return NearestSchedulingGroup();
  } else if (id == Fiber::kUnspecifiedSchedulingGroup) {
    return GetSchedulingGroup(
        Random(fiber::GetSchedulingGroupCount()));
  } else {
    return GetSchedulingGroup(id);
  }
}

} // namespace 

Fiber::Fiber() = default;

Fiber::~Fiber() {
  CHECK(!joinable()) << "You need to call either `join()` or `detach()` \
                                prior to destroy a fiber.";
}

Fiber::Fiber(UniqueFunction<void()>&& start) {
  auto sg = NearestSchedulingGroup();
  CHECK(sg) << "No scheduling group is available?";

  // TODO: implement ExitBarrier
  auto fiberEntity = CreateFiberEntity(sg, std::move(start), std::make_shared<ExitBarrier>());
  sg->StartFiber(fiberEntity);
}

// Fiber::Fiber(ExecutionContext* context, UniqueFunction<void()>&& start) {
//   auto sg = NearestSchedulingGroup();
//   CHECK(sg) << "No scheduling group is available?";

//   if (context) {
//     start = [start = std::move(start),
//              ec = std::make_shared<ExecutionContext>()] {
//       // TODO: implement ExecutionContext.
//       ec->Execute(start);
//     };
//   }
//   // TODO: implement ExitBarrier
//   auto fiberEntity = CreateFiberEntity(sg, std::move(start), std::make_shared<ExitBarrier>());
//   sg->StartFiber(fiberEntity);
// }

void Fiber::detach() {
  CHECK(joinable())<<  "The fiber is in detached state.";
  joinImpl_ = nullptr;
}

void Fiber::join() {
  CHECK(joinable()) << "The fiber is in detached state.";
  joinImpl_->Wait();
  joinImpl_->Reset();
}

bool Fiber::joinable() const { return !!joinImpl_; }

Fiber::Fiber(Fiber&&) noexcept = default;
Fiber& Fiber::operator=(Fiber&&) noexcept = default;


void StartFiberFromPthread(UniqueFunction<void()>&& start_proc) {
  StartFiberDetached(std::move(start_proc));
}

void StartFiberDetached(UniqueFunction<void()>&& start_proc) {
  auto sg = NearestSchedulingGroup();
  auto fiberEntity = CreateFiberEntity(sg, std::move(start_proc));
  
  CHECK(!fiberEntity->exitBarrier_);

  sg->StartFiber(fiberEntity);
}

// void StartFiberDetached(ExecutionContext* context, UniqueFunction<void()>&& start_proc) {
//   auto sg = NearestSchedulingGroup();

//   if (context) {
//     start_proc = [start_proc = std::move(start_proc),
//                   ec = std::make_shared<ExitBarrier>()] {
//       ec->Execute(start_proc);
//     };
//   }

//   auto fiberEntity = CreateFiberEntity(sg, std::move(start_proc));
//   CHECK(!fiberEntity->exitBarrier_);
//     sg->StartFiber(fiberEntity);
// }

} // namespace tinyRPC::fiber
