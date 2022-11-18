// clang-format off
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

template <class F>
struct not_fn_t {
  F f;

  template <class Self, class... Args>
  constexpr auto operator()(this Self&& self, Args&&... args) noexcept(
    noexcept(   !std::invoke(std::forward_like<Self>(self.f), std::forward<Args>(args)...)))
    -> decltype(!std::invoke(std::forward_like<Self>(self.f), std::forward<Args>(args)...)) {
    return      !std::invoke(std::forward_like<Self>(self.f), std::forward<Args>(args)...);
  }
};

template <class F>
constexpr not_fn_t<std::decay_t<F>> my_not_fn(F&& f) {
  return {std::forward<F>(f)};
}

int main()
{
  constexpr auto non_empty = my_not_fn(&std::string::empty);
  std::string str = "str";
  std::cout << std::boolalpha << non_empty(str) << std::endl;
}
