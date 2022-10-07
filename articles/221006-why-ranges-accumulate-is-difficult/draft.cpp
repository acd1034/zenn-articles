// #include <cassert>
#include <concepts>
#include <iterator>
// #include <ranges>
#include <type_traits>
#include <utility>
using namespace std;

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

template <class... Args>
struct accumulate_result;

template <class Op, class T, class U>
concept magma =
  common_with<T, U> &&
  regular_invocable<Op, T, T> &&
  regular_invocable<Op, U, U> &&
  regular_invocable<Op, T, U> &&
  regular_invocable<Op, U, T> &&
  common_with<invoke_result_t<Op&, T, U>, T> &&
  common_with<invoke_result_t<Op&, T, U>, U> &&
  same_as<invoke_result_t<Op&, T, U>, invoke_result_t<Op&, U, T>>;

template <class Op, class I1, class I2, class O>
concept indirect_magma =
  indirectly_readable<I1> &&
  indirectly_readable<I2> &&
  indirectly_writable<O, indirect_result_t<Op&, I1, I2>> &&
  magma<Op&, iter_value_t<I1>&, iter_value_t<I2>&> &&
  magma<Op&, iter_value_t<I1>&, iter_reference_t<I2>&> &&
  magma<Op&, iter_reference_t<I1>, iter_value_t<I2>&> &&
  magma<Op&, iter_reference_t<I1>, iter_reference_t<I2>> &&
  magma<Op&, iter_common_reference_t<I1>, iter_common_reference_t<I2>>;

template <input_iterator I, sentinel_for<I> S, movable T, class Proj = identity,
          indirect_magma<const T*, projected<I, Proj>, T*> Op = plus<>>
constexpr accumulate_result<I, T>
accumulate(I first, S last, T init, Op op = {}, Proj proj = {});

// clang-format on

template <class T>
concept Subtractable = requires(T a, T b) {
  a - b;
};
