#ifndef _TINYRPC_SRC_IO_DETAIL_WATCHDOG_H_
#define _TINYRPC_SRC_IO_DETAIL_WATCHDOG_H_

#include <atomic>
#include <thread>
#include <vector>
#include <latch>

namespace tinyRPC {

class EventLoop;

}  // namespace tinyRPC

namespace tinyRPC::io::detail {

class Watchdog {
 public:
  Watchdog();

  void AddEventLoop(EventLoop* watched);

  void Start();
  void Stop();

  void Join();

 private:
  void WorkerProc();

 private:
  std::atomic<bool> exiting_{false};
  std::latch latch_{1};  
  std::vector<EventLoop*> watched_;
  std::thread watcher_;
};

}  // namespace tinyRPC::io::detail

#endif  