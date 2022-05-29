#ifndef _SRC_RPC_NAME_RESOLVER_NAME_RESOLVER_UPDATER_H_
#define _SRC_RPC_NAME_RESOLVER_NAME_RESOLVER_UPDATER_H_

#include <atomic>
#include <chrono>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "../../base/Function.h"

namespace tinyRPC::name_resolver {

// A tool class to update route table periodically.
class NameResolverUpdater {
  struct UpdaterInfo {
    UpdaterInfo(UniqueFunction<void()> up, std::chrono::seconds inter)
        : updater(std::move(up)), interval(inter) {
      update_time = std::chrono::steady_clock::now();
    }
    UniqueFunction<void()> updater;
    const std::chrono::seconds interval;
    std::chrono::steady_clock::time_point update_time;
  };

 public:
  NameResolverUpdater();
  ~NameResolverUpdater();
  void Stop();
  void Register(const std::string& name, UniqueFunction<void()> updater,
                std::chrono::seconds interval);

 private:
  void Start();
  void WorkProc();

 private:
  std::mutex updater_mutex_;
  std::condition_variable cond_;
  std::unordered_map<std::string, UpdaterInfo> updater_;

  std::atomic<bool> stopped_{false};
  std::thread work_waiter_;
};

}  // namespace tinyRPC::name_resolver

#endif  
