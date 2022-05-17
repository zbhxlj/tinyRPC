#ifndef _SRC_BASE_RANDOM_H_
#define _SRC_BASE_RANDOM_H_

#include <random>
#include <limits>

namespace tinyRPC{

template <typename T>
T Random(T val){
    std::srand(time(NULL));
    return rand() % val;
}

} // namespace tinyRPC

#endif