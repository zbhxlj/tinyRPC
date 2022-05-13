#ifndef _SRC_BASE_THREAD_H_
#define _SRC_BASE_THREAD_H_

#include <atomic>

namespace tinyRPC{

// Simple spinlock implementation.

class SpinLock{
public:
    SpinLock() = default;
    ~SpinLock() = default;

    // Ugly lowercase to use `unique_lock`.
    void lock() noexcept{
        // First try to grab lock.
        if(tryLock()){
            return;
        }

        // Lock might be contend, spin.
        LockSlow();
    }

    void unlock() noexcept{
        locked_.store(false);
    }

    bool tryLock() noexcept{
        return !locked_.exchange(true);
    }

    void LockSlow() noexcept{
        while(locked_.exchange(true)){
            asm volatile(
            "pause":
                   :
                   :
            "memory");
        }
    }
private:
    std::atomic<bool> locked_{false};
};
}

#endif