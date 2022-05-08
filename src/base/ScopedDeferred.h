#ifndef _SRC_BASE_SCOPED_DEFERRED_H_
#define _SRC_BASE_SCOPED_DEFERRED_H_

#include <utility>

namespace tinyRPC{

template<typename F>
class ScopedDeferred{
public:
    explicit ScopedDeferred(F&& f) : func_(std::move(f)){}
    // Nonmovable.
    ~ScopedDeferred() { func_(); }

    // Noncopyable.
    ScopedDeferred(const ScopedDeferred&) = delete;
    ScopedDeferred& operator=(const ScopedDeferred&) = delete;
private:
    F func_;
};

}

#endif