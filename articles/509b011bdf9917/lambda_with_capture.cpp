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

template <class Op, class ...Args>
struct perfect_forwarded_t : __perfect_forward<Op, Args...> {
    using __perfect_forward<Op, Args...>::__perfect_forward;
};

int main() {
  using affine_transform_op = decltype([b = 1.0](double a, double x) { return a * x + b; });
  perfect_forwarded_t<affine_transform_op, double> affine_transform(2.0);
  std::cout << affine_transform(3.0) << std::endl; // error!
}
