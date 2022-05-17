#include <vector>
#include <memory>
#include <algorithm>

#include "../../include/gflags/gflags.h"

#include "Runtime.h"
#include "detail/SchedulingGroup.h"
#include "detail/FiberWorker.h"

namespace tinyRPC::fiber{

DEFINE_int32(SchedulingGroupCnt, 4, "");

DEFINE_int32(SchedulingGroupWorkerSize, 4,
             "Internally Flare divides worker threads into groups, and tries "
             "to avoid sharing between them. This option controls group size "
             "of workers. Setting it too small may result in unbalanced "
             "workload, setting it too large can hurt overall scalability.");

DEFINE_int32(flare_work_stealing_ratio, 16,
             "Reciprocal of ratio for stealing job from other scheduling "
             "groups in same NUMA domain. Note that if multiple \"foreign\" "
             "scheduling groups present, the actual work stealing ratio is "
             "multiplied by foreign scheduling group count.");


struct FullyFledgedSchedulingGroup {
  std::unique_ptr<detail::SchedulingGroup> schedulingGroup;
  std::vector<std::unique_ptr<detail::FiberWorker>> fiberWorkers;
  std::unique_ptr<detail::TimerWorker> timerWorker;

  void Start() {
    timerWorker->Start();
    for (auto&& e : fiberWorkers) {
      e->Start();
    }
  }

  void Stop() {
    timerWorker->Stop();
    schedulingGroup->Stop();
  }

  void Join() {
    timerWorker->Join();
    for (auto&& e : fiberWorkers) {
      e->Join();
    }
  }
};

std::vector<std::unique_ptr<FullyFledgedSchedulingGroup>> flattenedSchedulingGroup;

std::unique_ptr<FullyFledgedSchedulingGroup> CreateFullyFledgedSchedulingGroup(std::size_t size) {
  auto rc = std::make_unique<FullyFledgedSchedulingGroup>();

  rc->schedulingGroup =
      std::make_unique<detail::SchedulingGroup>(size);
  for (int i = 0; i != size; ++i) {
    rc->fiberWorkers.push_back(
        std::make_unique<detail::FiberWorker>(rc->schedulingGroup.get(), i));
  }
  rc->timerWorker =
      std::make_unique<detail::TimerWorker>(rc->schedulingGroup.get());

  rc->schedulingGroup->SetTimerWorker(rc->timerWorker.get());
  return rc;
}

void InitializeForeignSchedulingGroups(
    const std::vector<std::unique_ptr<FullyFledgedSchedulingGroup>>& thieves,
    const std::vector<std::unique_ptr<FullyFledgedSchedulingGroup>>& victims,
    std::uint64_t steal_every_n) {
  for (std::size_t thief = 0; thief != thieves.size(); ++thief) {
    for (std::size_t victim = 0; victim != victims.size(); ++victim) {
      if (thieves[thief]->schedulingGroup ==
          victims[victim]->schedulingGroup) {
        return;
      }
      for (auto&& e : thieves[thief]->fiberWorkers) {
          e->AddForeignSchedulingGroup(victims[victim]->schedulingGroup.get(),
                                       steal_every_n);
      }
    }
  }
}

void StartWorkersUma() {
  LOG(INFO) << "Starting "<< FLAGS_SchedulingGroupWorkerSize <<  " worker threads per group, for a total of " 
            << FLAGS_SchedulingGroupCnt <<  " groups. The system is treated as UMA.";

  for (std::size_t index = 0; index != FLAGS_SchedulingGroupCnt;
       ++index) {
    flattenedSchedulingGroup.push_back(CreateFullyFledgedSchedulingGroup(FLAGS_SchedulingGroupWorkerSize));
}

  InitializeForeignSchedulingGroups(flattenedSchedulingGroup, flattenedSchedulingGroup,
                                    FLAGS_flare_work_stealing_ratio);
}

[[maybe_unused]] std::size_t GetCurrentSchedulingGroupIndex() {
  auto rc = NearestSchedulingGroupIndex();
  CHECK(rc != -1) <<
              "Calling `GetCurrentSchedulingGroupIndex` outside of any "
              "scheduling group is undefined.";
  return rc;
}

void StartRuntime() {
  StartWorkersUma();

  for (auto&& e : flattenedSchedulingGroup) {
      e->Start();
  }
}

[[maybe_unused]] void TerminateRuntime() {
  for (auto&& e : flattenedSchedulingGroup) {
    e->Stop();
  }
  for (auto&& e : flattenedSchedulingGroup) {
    e->Join();
  }

  flattenedSchedulingGroup.clear();
}

[[maybe_unused]] std::size_t GetSchedulingGroupCount() {
  return flattenedSchedulingGroup.size();
}

[[maybe_unused]] std::size_t GetSchedulingGroupWorkerSize() {
  return FLAGS_SchedulingGroupWorkerSize;
}

[[maybe_unused]] SchedulingGroup* GetSchedulingGroup(std::size_t index) {
  CHECK_LT(index, flattenedSchedulingGroup.size());
  return flattenedSchedulingGroup[index]->schedulingGroup.get();
}

[[maybe_unused]] SchedulingGroup* NearestSchedulingGroupSlow(SchedulingGroup** cache) {
  if (auto rc = SchedulingGroup::Current(); rc) {
    *cache = rc;
    return rc;
  }

  std::srand(time(NULL));
  thread_local std::size_t next = std::rand();


  if (!flattenedSchedulingGroup.empty()) {
    return flattenedSchedulingGroup[next++ % flattenedSchedulingGroup.size()]->schedulingGroup.get();
  }

  return nullptr;
}

[[maybe_unused]] std::ptrdiff_t NearestSchedulingGroupIndex() {
  thread_local auto cached = []() -> std::ptrdiff_t {
    auto sg = NearestSchedulingGroup();
    if (sg) {
      auto iter = std::find_if(
          flattenedSchedulingGroup.begin(), flattenedSchedulingGroup.end(),
          [&](auto&& e) { return e->schedulingGroup.get() == sg; });
      CHECK(iter != flattenedSchedulingGroup.end());
      return iter - flattenedSchedulingGroup.begin();
    }
    return -1;
  }();
  return cached;
}

} // namespace tinyRPC::fiber
