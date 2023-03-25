// clang-format off
#include <charconv>
#include <type_traits>
#include <utility>
// for main
#include <format>
#include <iostream>
#include <string>

namespace ns {
  template <char... Chars>
  constexpr std::size_t parse_ic() {
    const char cs[]{Chars...};
    std::size_t n{};
    auto [ptr, ec] = std::from_chars(std::begin(cs), std::end(cs), n);
    return ptr == std::end(cs) && ec == std::errc{} ? n : throw std::logic_error("");
  }

  template <char... Chars>
  constexpr auto operator""_ic() {
    return std::integral_constant<std::size_t, parse_ic<Chars...>()>{};
  }

  template <std::size_t I, class T>
  struct tuple_leaf {
    T value;
  };

  template <std::size_t I, class T>
  tuple_leaf<I, T> at_index(const tuple_leaf<I, T>&); // undefined

  template <class Seq, class... Ts>
  struct tuple;

  template <std::size_t... Is, class... Ts>
  struct tuple<std::index_sequence<Is...>, Ts...> : tuple_leaf<Is, Ts>... {
    template <std::size_t I, class Self>
    requires (I < sizeof...(Ts))
    constexpr decltype(auto) operator[](this Self&& self, std::integral_constant<std::size_t, I>) {
      using leaf = decltype(at_index<I>(self));
      return std::forward_like<Self>(static_cast<leaf&>(self).value);
    }
  };

  template <class... Ts>
  tuple(Ts...) -> tuple<std::index_sequence_for<Ts...>, Ts...>;
} // namespace ns

int main() {
  using ns::operator""_ic;
  ns::tuple t{1, 3.14, std::string("hello")};
  std::cout << std::format("({}, {}, {})\n",
                           t[0_ic],
                           t[1_ic],
                           t[2_ic]); // (1, 3.14, "hello") を出力
}
