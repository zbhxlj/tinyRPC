#ifndef _SRC_BASE_FUNCTION_H_
#define _SRC_BASE_FUNCTION_H_

#include <functional>
#include <iostream>
#include <type_traits>
#include <utility>

namespace tinyRPC{

// Implementation for std::function storing move-captured lambda expression. 
template<typename T>
class UniqueFunction : public std::function<T>
{
    template<typename Fn, typename En = void>
    struct wrapper;

    // specialization for CopyConstructible Fn
    template<typename Fn>
    struct wrapper<Fn, std::enable_if_t< std::is_copy_constructible<Fn>::value >>
    {
        Fn fn;

        template<typename... Args>
        auto operator()(Args&&... args) { return fn(std::forward<Args>(args)...); }
    };

    // specialization for MoveConstructible-only Fn
    template<typename Fn>
    struct wrapper<Fn, std::enable_if_t< !std::is_copy_constructible<Fn>::value
        && std::is_move_constructible<Fn>::value >>
    {
        Fn fn;

        wrapper(Fn&& fn) : fn(std::forward<Fn>(fn)) { }

        wrapper(wrapper&&) = default;
        wrapper& operator=(wrapper&&) = default;

        // these two functions are instantiated by std::function
        // and are never called
        wrapper(const wrapper& rhs) : fn(const_cast<Fn&&>(rhs.fn)) { throw 0; } // hack to initialize fn for non-DefaultContructible types
        wrapper& operator=(wrapper&) { throw 0; }

        template<typename... Args>
        auto operator()(Args&&... args) { return fn(std::forward<Args>(args)...); }
    };

    using base = std::function<T>;

public:
    UniqueFunction() noexcept = default;
    UniqueFunction(std::nullptr_t) noexcept : base(nullptr) { }

    template<typename Fn>
    UniqueFunction(Fn&& f) : base(wrapper<Fn>{ std::forward<Fn>(f) }) { }

    UniqueFunction(UniqueFunction&&) = default;
    UniqueFunction& operator=(UniqueFunction&&) = default;

    UniqueFunction& operator=(std::nullptr_t) { base::operator=(nullptr); return *this; }

    template<typename Fn>
    UniqueFunction& operator=(Fn&& f)
    { base::operator=(wrapper<Fn>{ std::forward<Fn>(f) }); return *this; }

    using base::operator();
};

} // namesapce tinyRPC

#endif