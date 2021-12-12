#include <utility>

template<std::size_t I, class... Types>
constexpr std::tuple_element_t<I, std::tuple<Types...>>&
get(std::tuple<Types...>& t) noexcept;

template<std::size_t I, class... Types>
constexpr std::tuple_element_t<I, std::tuple<Types...>>&&
get(std::tuple<Types...>&& t) noexcept;

template<std::size_t I, class... Types>
constexpr const std::tuple_element_t<I, std::tuple<Types...>>&
get(const std::tuple<Types...>& t) noexcept;

template<std::size_t I, class... Types>
constexpr const std::tuple_element_t<I, std::tuple<Types...>>&&
get(const std::tuple<Types...>&& t) noexcept;
