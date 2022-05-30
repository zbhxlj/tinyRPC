#include "NameResolverUpdater.h"

#include <string>
#include <utility>
#include <vector>

#include "../../base/Logging.h"
#include "../../base/base.h"

using namespace std::literals;

namespace tinyRPC::name_resolver {

NameResolverUpdater::NameResolverUpdater() { Start(); }

NameResolverUpdater::~NameResolverUpdater() { Stop(); }

void NameResolverUpdater::Stop() {
  if (!std::atomic_exchange(&stopped_, true)) {
    cond_.notify_one();
    work_waiter_.join();
  }
}

void NameResolverUpdater::Start() {
  work_waiter_ = std::thread([this]() {
    SetCurrentThreadName("NameResolverUp");
    WorkProc();
  });
}

void NameResolverUpdater::Register(const std::string& address,
                                   UniqueFunction<void()> updater,
                                   std::chrono::seconds interval) {
  std::scoped_lock lk(updater_mutex_);
  if (auto iter = updater_.find(address); iter == updater_.end()) {
    updater_.insert(
        std::make_pair(address, UpdaterInfo(std::move(updater), interval)));
  } else {
    FLARE_LOG_ERROR("Duplicate Register for address {}", address);
  }
}

void NameResolverUpdater::WorkProc() {
  while (!stopped_) {
    auto now = std::chrono::steady_clock::now();
    std::vector<UpdaterInfo*> need_update;
    {
      std::scoped_lock lk(updater_mutex_);
      for (auto&& [address, updater_info] : updater_) {
        if (now - updater_info.update_time > updater_info.interval) {
          need_update.push_back(&updater_info);
        }
      }
    }
    if (!need_update.empty()) {
      for (auto&& updater_info : need_update) {
        updater_info->updater();  // may be blocking
        updater_info->update_time = std::chrono::steady_clock::now();
      }
    }
    std::unique_lock lk(updater_mutex_);
    cond_.wait_for(lk, 1s,
                   [this] { return stopped_.load(); });
  }
}

}  // namespace tinyRPC::name_resolver
