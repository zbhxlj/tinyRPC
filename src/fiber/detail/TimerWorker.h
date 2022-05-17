#ifndef _SRC_FIBER_DETAIL_TIMER_WORKER_H_
#define _SRC_FIBER_DETAIL_TIMER_WORKER_H_

#include <chrono>
#include <mutex>
#include <memory>
#include <atomic>
#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <condition_variable>
#include <latch>

#include "../../base/SpinLock.h"
#include "../../base/Function.h"

namespace tinyRPC::fiber::detail{

class TimerWorker;
class SchedulingGroup;
struct Timer;
using TimerPtr = std::shared_ptr<Timer>;

struct Timer{
    SpinLock lock_;
    std::atomic<bool> cancelled_ {false};
    bool periodic_ = false;
    TimerWorker* owner_ {nullptr};
    UniqueFunction<void(TimerPtr& )> cb_;
    std::chrono::steady_clock::time_point expiresAt_;
    std::chrono::nanoseconds interval_;
};

struct TimerCmp{
    bool operator()(const TimerPtr& a, const TimerPtr& b){
        return a->expiresAt_ > b->expiresAt_;
    }
};

struct ThreadLocalQueue{
    SpinLock lock_;
    std::vector<TimerPtr> timers_;
    std::chrono::steady_clock::time_point expiresAt_ = 
        std::chrono::steady_clock::time_point::max();
};

class TimerWorker{
public:
    explicit TimerWorker(SchedulingGroup* sg);
    
    ~TimerWorker();

    static TimerWorker* GetTimerOwner(TimerPtr& );

    TimerPtr CreateTimer(std::chrono::steady_clock::time_point expires_at,
                            UniqueFunction<void(TimerPtr&)>&& cb);
    TimerPtr CreateTimer(
      std::chrono::steady_clock::time_point initial_expires_at,
      std::chrono::nanoseconds interval, UniqueFunction<void(TimerPtr&)>&& cb);

    // Enable a timer created before.
    void EnableTimer(TimerPtr& timer);

    // Cancel a timer.
    void RemoveTimer(TimerPtr& timer);

  // Detach a timer. This method can be helpful in fire-and-forget use cases.
//   void DetachTimer(std::uint64_t timer_id);

    SchedulingGroup* GetSchedulingGroup();

    void Start();

    void Stop();

    void Join();

    TimerWorker(const TimerWorker&) = delete;
    
    TimerWorker& operator=(const TimerWorker&) = delete;
    
    void InitializeLocalQueue(std::size_t worker_index);

    ThreadLocalQueue* GetThreadLocalQueue();
private:    
    void WorkerProc();

    void AddTimer(TimerPtr timer);

    void WaitForWorkers();

    void ReapThreadLocalQueues();
    void FireTimers();
    void WakeWorkerIfNeeded(std::chrono::steady_clock::time_point local_expires_at);

    SchedulingGroup* sg_;

    std::priority_queue<TimerPtr, std::vector<TimerPtr>, TimerCmp> timers_;
    std::vector<ThreadLocalQueue*> localQueues_;
    std::chrono::steady_clock::duration nextExpireAt = std::chrono::steady_clock::duration::max();
    std::atomic<bool> stopped_ {false};
    std::latch latch_;
    std::thread worker_;
    // Sleep on this.
    std::mutex lock_;
    std::condition_variable cv_;
};

} // namespace tinyRPC::fiber::detail

#endif