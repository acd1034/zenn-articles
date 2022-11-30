#include <cassert>
#include <compare>
#include <concepts>
#include <functional>
#include <iterator>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>
// for test
#include <cmath>
#include <stdexcept>

namespace ns {
  template <class T, class U>
  using __forward_like_t = decltype(std::forward_like<T>(std::declval<U>()));

  template <class T>
  struct optional;

  template <class T>
  inline constexpr bool __is_optional_v = false;
  template <class T>
  inline constexpr bool __is_optional_v<optional<T>> = true;

  template <class T>
  struct optional : std::optional<T> {
    using std::optional<T>::optional;

    friend constexpr auto operator<=>(const optional&,
                                      const optional&) = default;

    template <class Self, std::invocable<__forward_like_t<Self, T>> F>
    constexpr auto transform(this Self&& self, F&& f)
      -> optional<std::remove_cvref_t<
        std::invoke_result_t<F&&, __forward_like_t<Self, T>>>> {
      if (self)
        return std::invoke(std::forward<F>(f), std::forward_like<Self>(*self));
      else
        return std::nullopt;
    }

    template <class Self, std::invocable<__forward_like_t<Self, T>> F>
    requires __is_optional_v<
      std::remove_cvref_t<std::invoke_result_t<F&&, __forward_like_t<Self, T>>>>
    constexpr auto and_then(this Self&& self, F&& f)
      -> decltype(std::invoke(std::forward<F>(f),
                              std::forward_like<Self>(*self))) {
      if (self)
        return std::invoke(std::forward<F>(f), std::forward_like<Self>(*self));
      else
        return std::nullopt;
    }

    template <class Self, std::invocable F>
    requires std::same_as<std::remove_cvref_t<std::invoke_result_t<F&&>>,
                          optional<T>>
    constexpr optional<T> or_else(this Self&& self, F&& f) {
      if (self)
        return std::forward<Self>(self);
      else
        return std::invoke(std::forward<F>(f));
    }
  };

  template <class T>
  optional(T) -> optional<T>;

  template <class T>
  constexpr bool operator==(const optional<T>& x, std::nullopt_t) {
    return not x;
  }
  template <class T>
  constexpr auto operator<=>(const optional<T>& x, std::nullopt_t) {
    return x.has_value() <=> false;
  }
} // namespace ns

#define FWD(x) static_cast<decltype(x)&&>(x)

int main() {
  {
    ns::optional o1(42);
    ns::optional<int> o2;
    auto add_one = [](auto&& x) { return FWD(x) + 1; };
    assert(o1.transform(add_one) == ns::optional(43));
    assert(o2.transform(add_one) == std::nullopt);
  }
  {
    ns::optional o1(42);
    ns::optional<int> o2;
    auto sqrt = [](int x) -> ns::optional<int> {
      if (x < 0)
        return std::nullopt;
      else
        return static_cast<int>(std::sqrt(x));
    };
    assert(o1.and_then(sqrt) == ns::optional(6));
    assert(o2.and_then(sqrt) == std::nullopt);
  }
  {
    ns::optional o1(42);
    ns::optional<int> o2;
    auto twelve = []() -> ns::optional<int> { return 12; };
    assert(o1.or_else(twelve) == ns::optional(42));
    assert(o2.or_else(twelve) == ns::optional(12));
  }
}
