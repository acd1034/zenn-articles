// clang-format off
#include <type_traits>
#include <utility>
// for main
#include <format>
#include <iostream>
#include <string>

namespace ns {
  template <std::size_t I, class T>
  struct tuple_leaf {
    T value;
  };

  template <std::size_t I, class T>
  constexpr tuple_leaf<I, T> at_index(const tuple_leaf<I, T>&); // undefined

  template <class Seq, class... Ts>
  struct tuple;

  template <std::size_t... Is, class... Ts>
  struct tuple<std::index_sequence<Is...>, Ts...> : tuple_leaf<Is, Ts>... {
    template <std::size_t I, class Self>
    requires (I < sizeof...(Ts))
    constexpr decltype(auto) get(this Self&& self) {
      using leaf = decltype(at_index<I>(self));
      return std::forward_like<Self>(static_cast<leaf&>(self).value);
    }
  };

  template <class... Ts>
  tuple(Ts...) -> tuple<std::index_sequence_for<Ts...>, Ts...>;
} // namespace ns

int main() {
  ns::tuple t{1, 3.14, std::string("hello")};
  std::cout << std::format("({}, {}, {})\n",
                           t.template get<0>(),
                           t.template get<1>(),
                           t.template get<2>()); // (0, 3.14, "hello") を出力
}
