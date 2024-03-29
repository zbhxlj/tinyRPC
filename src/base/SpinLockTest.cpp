#include <thread>
// #include <latch>

#include "gtest/gtest.h"

#include "Latch.h"
#include "SpinLock.h"

namespace tinyRPC{

    TEST(SpinLock, All){
        SpinLock spinLock;
        std::uint64_t counter = 0;

        constexpr std::uint64_t threadCounter = 10000;
        constexpr std::uint64_t totalCounter = 100000;

        std::thread threads[10];
        Latch latch(10);

        for(auto&& t : threads){
            t = std::thread([&latch, &counter, &spinLock](){
                latch.count_down();
                for(auto i = threadCounter; i > 0; i--){
                    spinLock.lock();
                    counter++;
                    spinLock.unlock();
                }
            });
        }

        latch.wait();
        for(auto&& t: threads){
            t.join();
        }

        ASSERT_EQ(counter, totalCounter);
    }
}