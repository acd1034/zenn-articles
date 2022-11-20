#include <ranges>
#include <vector>

// ---------------------------
template <class R, class Pred>
requires std::ranges::input_range<R> and std::predicate<Pred&, std::ranges::range_reference_t<R>>
constexpr bool all_of(R&&, Pred);

template <std::ranges::input_range R, std::predicate<std::ranges::range_reference_t<R>> Pred>
constexpr bool all_of(R&&, Pred);

// ---------------------------
inline constexpr auto do_nothing = [](anything auto&& x) {
  return static_cast<decltype(x)&&>(x);
};

// ---------------------------
template <typename... Args>
class absolute {
  static_assert(nothing<Args...>);
};

// ---------------------------
template <class T, class U>
concept similar_to = std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

// ---------------------------
constexpr auto do_something(similar_to<std::string> auto&& str) {

}

// ---------------------------
#include <ranges>
#include <vector>
template <class T>
concept rvalue_range = rvalue<T> and std::ranges::range<T>;

constexpr auto accumulate(std::ranges::range auto& r, auto init) {
  return std::fold_left(r, std::move(init), std::ranges::plus{});
}

constexpr auto accumulate(rvalue_range auto&& r, auto init) {
  return std::fold_left(r | std::views::as_rvalue, std::move(init), std::ranges::plus{});
}

int main() {
  accumulate(std::vector{0, 1, 2}, 3);
}
