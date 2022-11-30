#include <optional>
#include <type_traits>
#include <utility>
// for main
#include <format>
#include <iostream>
#include <string>

namespace ns {
  template <class T>
  inline constexpr bool is_optional_v = false;
  template <class T>
  inline constexpr bool is_optional_v<std::optional<T>> = true;

  template <class T>
  concept optional_ = is_optional_v<std::remove_cvref_t<T>>;

  template <optional_ F, optional_ O>
  constexpr auto apply(F&& f, O&& o)
    -> decltype(std::forward<O>(o).transform(*std::forward<F>(f))) {
    if (f)
      return std::forward<O>(o).transform(*std::forward<F>(f));
    else
      return std::nullopt;
  }

  template <std::size_t... Is, class... Args>
  constexpr auto lift_impl(std::index_sequence<Is...>, Args&&... args) {
    auto t = std::forward_as_tuple(std::forward<Args>(args)...);
    if constexpr (sizeof...(Args) == 2)
      return std::get<1>(std::move(t)).transform(std::get<0>(std::move(t)));
    else
      return apply(lift_impl(std::make_index_sequence<sizeof...(Is) - 1>{}, std::get<Is>(std::move(t))...), std::get<sizeof...(Is)>(std::move(t)));
  }

  template <class... Args>
  requires (sizeof...(Args) > 1)
  constexpr auto lift(Args&&... args) {
    return lift_impl(std::make_index_sequence<sizeof...(Args) - 1>{}, std::forward<Args>(args)...);
  }
}

int main() {
  std::optional o1(1);
  std::optional o2(3.14);
  std::optional o3(std::string("hello"));
  auto fn = [](int i, double d, const std::string& s) {
    return std::format("({}, {}, {})", i, d, s);
  };
  if (auto result = ns::lift(fn, o1, o2, o3); result)
    std::cout << *result << std::endl;
}
