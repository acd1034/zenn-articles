#include <functional>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>

template <class Op, class Indices, class ...Bound>
struct __perfect_forward_impl;

template <class Op, std::size_t ...Idx, class ...Bound>
struct __perfect_forward_impl<Op, std::index_sequence<Idx...>, Bound...> {
private:
    std::tuple<Bound...> bound_;

public:
    template <class ...BoundArgs, class = std::enable_if_t<
        std::is_constructible_v<std::tuple<Bound...>, BoundArgs&&...>
    >>
    explicit constexpr __perfect_forward_impl(BoundArgs&& ...bound)
        : bound_(std::forward<BoundArgs>(bound)...)
    { }

    __perfect_forward_impl(__perfect_forward_impl const&) = default;
    __perfect_forward_impl(__perfect_forward_impl&&) = default;

    __perfect_forward_impl& operator=(__perfect_forward_impl const&) = default;
    __perfect_forward_impl& operator=(__perfect_forward_impl&&) = default;

    template <class ...Args, class = std::enable_if_t<std::is_invocable_v<Op, Bound&..., Args...>>>
    constexpr auto operator()(Args&&... args) &
        noexcept(noexcept(Op()(std::get<Idx>(bound_)..., std::forward<Args>(args)...)))
        -> decltype(      Op()(std::get<Idx>(bound_)..., std::forward<Args>(args)...))
        { return          Op()(std::get<Idx>(bound_)..., std::forward<Args>(args)...); }

    template <class ...Args, class = std::enable_if_t<!std::is_invocable_v<Op, Bound&..., Args...>>>
    auto operator()(Args&&...) & = delete;

    template <class ...Args, class = std::enable_if_t<std::is_invocable_v<Op, Bound const&..., Args...>>>
    constexpr auto operator()(Args&&... args) const&
        noexcept(noexcept(Op()(std::get<Idx>(bound_)..., std::forward<Args>(args)...)))
        -> decltype(      Op()(std::get<Idx>(bound_)..., std::forward<Args>(args)...))
        { return          Op()(std::get<Idx>(bound_)..., std::forward<Args>(args)...); }

    template <class ...Args, class = std::enable_if_t<!std::is_invocable_v<Op, Bound const&..., Args...>>>
    auto operator()(Args&&...) const& = delete;

    template <class ...Args, class = std::enable_if_t<std::is_invocable_v<Op, Bound..., Args...>>>
    constexpr auto operator()(Args&&... args) &&
        noexcept(noexcept(Op()(std::get<Idx>(std::move(bound_))..., std::forward<Args>(args)...)))
        -> decltype(      Op()(std::get<Idx>(std::move(bound_))..., std::forward<Args>(args)...))
        { return          Op()(std::get<Idx>(std::move(bound_))..., std::forward<Args>(args)...); }

    template <class ...Args, class = std::enable_if_t<!std::is_invocable_v<Op, Bound..., Args...>>>
    auto operator()(Args&&...) && = delete;

    template <class ...Args, class = std::enable_if_t<std::is_invocable_v<Op, Bound const..., Args...>>>
    constexpr auto operator()(Args&&... args) const&&
        noexcept(noexcept(Op()(std::get<Idx>(std::move(bound_))..., std::forward<Args>(args)...)))
        -> decltype(      Op()(std::get<Idx>(std::move(bound_))..., std::forward<Args>(args)...))
        { return          Op()(std::get<Idx>(std::move(bound_))..., std::forward<Args>(args)...); }

    template <class ...Args, class = std::enable_if_t<!std::is_invocable_v<Op, Bound const..., Args...>>>
    auto operator()(Args&&...) const&& = delete;
};

// __perfect_forward implements a perfect-forwarding call wrapper as explained in [func.require].
template <class Op, class ...Args>
using __perfect_forward = __perfect_forward_impl<Op, std::index_sequence_for<Args...>, Args...>;

/*
struct partially_applied_plus_op {
    template <class T, class U>
    constexpr auto operator()(T&& t, U&& u) const noexcept(
        noexcept(std::forward<T>(t) + std::forward<U>(u)))
        -> decltype(std::forward<T>(t) + std::forward<U>(u)) {
        return std::forward<T>(t) + std::forward<U>(u);
    }
};
*/

using partially_applied_plus_op = std::plus<>;

template <class T>
struct partially_applied_plus_t : __perfect_forward<partially_applied_plus_op, T> {
    using __perfect_forward<partially_applied_plus_op, T>::__perfect_forward;
};

template <class T, class = std::enable_if_t<
    std::is_constructible_v<std::decay_t<T>, T> &&
    std::is_move_constructible_v<std::decay_t<T>>
>>
constexpr auto partially_applied_plus(T&& t) {
    return partially_applied_plus_t<std::decay_t<T>>(std::forward<T>(t));
}

int main() {
    constexpr auto fn = partially_applied_plus(42);
    std::cout << fn(1) << std::endl; // 43
}
