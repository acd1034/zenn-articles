#include <__config>
#include <__functional/perfect_forward.h>
#include <__functional/invoke.h>
#include <utility>

struct not_fn_op {
    template <class... Args>
    constexpr auto operator()(Args&&... args) const
        noexcept(noexcept(!std::invoke(std::forward<Args>(args)...)))
        -> decltype(      !std::invoke(std::forward<Args>(args)...))
        { return          !std::invoke(std::forward<Args>(args)...); }
};

template <class F>
struct not_fn_t : __perfect_forward<not_fn_op, F> {
    using __perfect_forward<not_fn_op, F>::__perfect_forward;
};

template <class F, class = std::enable_if_t<
    std::is_constructible_v<std::decay_t<F>, F> &&
    std::is_move_constructible_v<std::decay_t<F>>
>>
constexpr auto not_fn(F&& f) {
    return not_fn_t<std::decay_t<F>>(std::forward<F>(f));
}

int main() {
  using true_fn_t = decltype([](auto&&) { return true; });
  true_fn_t fn{};
  not_fn(fn);          // F = true_fn_t& → dangling reference may arise.
  not_fn(true_fn_t{}); // F = true_fn_t
}
