#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

template <class T, class U>
using __copy_const_t = std::conditional_t<std::is_const_v<T>, const U, U>;

template <class, class U>
struct __override_ref : std::remove_reference<U> {};
template <class T, class U>
struct __override_ref<T&, U> : std::add_lvalue_reference<U> {};
template <class T, class U>
struct __override_ref<T&&, U>
  : std::add_rvalue_reference<std::remove_reference_t<U>> {};
template <class T, class U>
using __override_ref_t = typename __override_ref<T, U>::type;

template <class T, class U>
using __forwarding_like_t = __override_ref_t<
  T, __copy_const_t<std::remove_reference_t<T>, std::remove_reference_t<U>>>;

template <class T>
[[nodiscard]] constexpr auto forward_like(auto&& x) noexcept
  -> __forwarding_like_t<T, decltype(x)>&& {
  return static_cast<__forwarding_like_t<T, decltype(x)>&&>(x);
}
