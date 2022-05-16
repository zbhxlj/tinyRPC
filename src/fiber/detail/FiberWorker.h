#ifndef _SRC_FIBER_DETAIL_FIBERWORKER_H_
#define _SRC_FIBER_DETAIL_FIBERWORKER_H_

#include <cstdint>
#include <queue>
#include <thread>

namespace tinyRPC::fiber::detail{
struct FiberEntity;
class SchedulingGroup;

// A pthread worker for running fibers.
class FiberWorker {
 public:
  FiberWorker(SchedulingGroup* sg, std::size_t worker_index);

  void AddForeignSchedulingGroup(SchedulingGroup* sg,
                                 std::uint64_t steal_every_n);

  void Start();

  void Join();

  FiberWorker(const FiberWorker&) = delete;
  FiberWorker& operator=(const FiberWorker&) = delete;

 private:
  void WorkerProc();
  FiberEntity* StealFiber();

 private:
  struct Victim {
    SchedulingGroup* sg;
    std::uint64_t steal_every_n;
    std::uint64_t next_steal;

    // `std::priority_queue` orders elements descendingly.
    bool operator<(const Victim& v) const { return next_steal > v.next_steal; }
  };

  SchedulingGroup* sg_;
  std::size_t workerIndex_;
  std::uint64_t stealVecClock_{};
  std::priority_queue<Victim> victims_;
  std::thread worker_;
};
}

#endif