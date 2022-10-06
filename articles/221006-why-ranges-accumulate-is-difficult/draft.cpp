// #include <cassert>
#include <concepts>
#include <iterator>
// #include <ranges>
#include <type_traits>
#include <utility>

template <class... Args>
concept __common_reference_with = true;

// clang-format off
template <class F, class I1, class I2>
concept indirectly_binary_invocable =
  std::indirectly_readable<I1> and
  std::indirectly_readable<I2> and
  std::copy_constructible<F> and
  std::invocable<F&, std::iter_value_t<I1>&, std::iter_value_t<I2>&> and
  std::invocable<F&, std::iter_value_t<I1>&, std::iter_reference_t<I2>> and
  std::invocable<F&, std::iter_reference_t<I1>, std::iter_value_t<I2>&> and
  std::invocable<F&, std::iter_reference_t<I1>, std::iter_reference_t<I2>> and
  std::invocable<F&, std::iter_common_reference_t<I1>, std::iter_common_reference_t<I2>> and
  __common_reference_with<
      std::invoke_result_t<F&, std::iter_value_t<I1>&, std::iter_value_t<I2>&>,
      std::invoke_result_t<F&, std::iter_value_t<I1>&, std::iter_reference_t<I2>>,
      std::invoke_result_t<F&, std::iter_reference_t<I1>, std::iter_value_t<I2>&>,
      std::invoke_result_t<F&, std::iter_reference_t<I1>, std::iter_reference_t<I2>>>;

template <std::input_iterator I, std::sentinel_for<I> S, class T,
          class Op = std::plus<>, class Proj = std::identity>
requires indirectly_binary_invocable<Op&, T*, std::projected<I, Proj>> and
  std::assignable_from<T&, std::indirect_result_t<Op&, T*, std::projected<I, Proj>>>
constexpr T accumulate(I first, S last, T init, Op op = {}, Proj proj = {});
