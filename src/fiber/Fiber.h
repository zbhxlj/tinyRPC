#ifndef _SRC_FIBER_FIBER_H_
#define _SRC_FIBER_FIBER_H_

#include <limits>
#include <memory>
#include <tuple>

#include "../base/Function.h"
#include "detail/FiberEntity.h"
#include "detail/Waitable.h"

namespace tinyRPC::fiber{

using detail::ExitBarrier;
// class ExecutionContext;

class Fiber{
public:
    static constexpr auto kNearestSchedulingGroup =
        std::numeric_limits<std::size_t>::max() - 1;
    static constexpr auto kUnspecifiedSchedulingGroup =
        std::numeric_limits<std::size_t>::max();

struct Attributes{
    std::size_t sg = kNearestSchedulingGroup;
    bool local {false};
};

    Fiber();
    ~Fiber();
    
    Fiber(const Attributes& attr, UniqueFunction<void()>&& start);

    template <class F, class... Args,
            class = std::enable_if_t<std::is_invocable_v<F&&, Args&&...>>>
    explicit Fiber(const Attributes& attr , F&& f, Args&&... args)
      : Fiber(attr, [f = std::forward<F>(f),
                        t = std::tuple(std::forward<Args>(args)...)] {
            std::apply(std::forward<F>(f), std::move(t));
            }) {}

    template <class F, class... Args,
            class = std::enable_if_t<std::is_invocable_v<F&&, Args&&...>>>
    explicit Fiber(F&& f, Args&&... args)
      : Fiber(Attributes{}, [f = std::forward<F>(f),
                        t = std::tuple(std::forward<Args>(args)...)] {
            std::apply(std::forward<F>(f), std::move(t));
            }) {}

    template <class F, class = std::enable_if_t<std::is_invocable_v<F&&>>>
    Fiber(const Attributes& attr, F&& f)
      : Fiber(attr, UniqueFunction<void()>(std::forward<F>(f))) {}

    template <class F, class = std::enable_if_t<std::is_invocable_v<F&&>>>
    Fiber(F&& f)
      : Fiber(Attributes{}, UniqueFunction<void()>(std::forward<F>(f))) {}

    // Fiber(ExecutionContext* context, UniqueFunction<void()>&& start);

    // template <class F, class... Args,
    //         class = std::enable_if_t<std::is_invocable_v<F&&, Args&&...>>>
    // explicit Fiber(F&& f, Args&&... args)
    //   : Fiber(nullptr, [f = std::forward<F>(f),
    //                     t = std::tuple(std::forward<Args>(args)...)] {
    //         std::apply(std::forward<F>(f)(std::move(t)));
    //         }) {}

    // template <class F, class... Args,
    //         class = std::enable_if_t<std::is_invocable_v<F&&, Args&&...>>>
    // Fiber(ExecutionContext* context, F&& f, Args&&... args)
    //     : Fiber(context, [f = std::forward<F>(f),
    //                     t = std::tuple(std::forward<Args>(args)...)] {
    //         std::apply(std::forward<F>(f)(std::move(t)));
    //         }) {}

    void detach();
    void join();
    bool joinable() const;
    Fiber(Fiber&&) noexcept;
    Fiber& operator=(Fiber&&) noexcept;

private:
    std::shared_ptr<ExitBarrier> joinImpl_;
};

void StartFiberFromPthread(UniqueFunction<void()>&& start_proc);
void StartFiberDetached(UniqueFunction<void()>&& start_proc);
// void StartFiberDetached(ExecutionContext*context, UniqueFunction<void()>&& start_proc);

} // namespace tinyRPC::fiber

#endif