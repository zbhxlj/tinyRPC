#ifndef _SRC_BASE_THREAD_H_
#define _SRC_BASE_THREAD_H_

#include <atomic>

namespace tinyRPC{

class SpinLock{
public:
    SpinLock() = default;
    ~SpinLock() = default;

    void Lock() noexcept{
        // First try to grab lock.
        if(TryLock()){
            return;
        }

        // Lock might be contend, spin.
        LockSlow();
    }

    void UnLock() noexcept{
        locked_.store(false);
    }

    bool TryLock() noexcept{
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