#include "ConditionalVariable.h"

namespace tinyRPC::fiber{

void ConditionVariable::notify_one() noexcept { impl_.notify_one(); }

void ConditionVariable::notify_all() noexcept { impl_.notify_all(); }

void ConditionVariable::wait(std::unique_lock<Mutex>& lock) {
  impl_.wait(lock);
}

} // namespace tinyRPC::fiber