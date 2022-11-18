// clang-format off
#include <functional>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ns {
  template <class F, class Seq, class... Bound>
  struct bind_back_t;

  template <class F, std::size_t... I, class... Bound>
  struct bind_back_t<F, std::index_sequence<I...>, Bound...> {
    F f;
    std::tuple<Bound...> bound;

    template <class Self, class... Args>
    constexpr auto operator()(this Self&& self, Args&&... args) noexcept(
      noexcept(   std::invoke(std::forward_like<Self>(self.f),
                              std::forward<Args>(args)...,
                              std::forward_like<Self>(std::get<I>(self.bound))...)))
      -> decltype(std::invoke(std::forward_like<Self>(self.f),
                              std::forward<Args>(args)...,
                              std::forward_like<Self>(std::get<I>(self.bound))...)) {
      return      std::invoke(std::forward_like<Self>(self.f),
                              std::forward<Args>(args)...,
                              std::forward_like<Self>(std::get<I>(self.bound))...);
    }
  };

  template <class F, class... Args>
  requires std::is_constructible_v<std::decay_t<F>, F> and
           std::is_move_constructible_v<std::decay_t<F>> and
           (std::is_constructible_v<std::decay_t<Args>, Args> and ...) and
           (std::is_move_constructible_v<std::decay_t<Args>> and ...)
  constexpr bind_back_t<std::decay_t<F>,
                        std::index_sequence_for<Args...>,
                        std::decay_t<Args>...>
  bind_back(F&& f, Args&&... args) {
    return {std::forward<F>(f), {std::forward<Args>(args)...}};
  }
}

int main() {
  constexpr auto minus_one = ns::bind_back(std::minus{}, 1);
  std::cout << minus_one(42) << std::endl;
}
