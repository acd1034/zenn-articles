#include <complex>
#include <concepts>
#include <functional>
#include <iostream>
#include <iterator>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace _ranges {
  template <class... Args>
  concept indirectly_binary_invocable = true;

  struct accumulate_fn {
    template <class I, class S, class T, class Op = std::plus<>,
              class P = std::identity>
    requires std::sentinel_for<S, I> and std::input_iterator<
      I> and indirectly_binary_invocable<Op, T*, std::projected<I, P>> and std::
      assignable_from<T&, std::indirect_result_t<Op&, T*, std::projected<I, P>>>
    constexpr T operator()(I first, S last, T init, Op op = Op{},
                           P proj = P{}) const {
      for (; first != last; ++first)
        init = std::invoke(op, std::move(init), std::invoke(proj, *first));
      return init;
    }

    template <class R, class T, class Op = std::plus<>, class P = std::identity>
    requires std::ranges::input_range<R> and indirectly_binary_invocable<
      Op, T*, std::projected<std::ranges::iterator_t<R>, P>> and std::
      assignable_from<T&,
                      std::indirect_result_t<
                        Op&, T*, std::projected<std::ranges::iterator_t<R>, P>>>
    constexpr T operator()(R&& r, T init, Op op = Op{}, P proj = P{}) const {
      return (*this)(std::ranges::begin(r), std::ranges::end(r),
                     std::move(init), std::move(op), std::move(proj));
    }
  };

  inline constexpr accumulate_fn accumulate{};
} // namespace _ranges

using namespace std;

int word_count(const vector<string>& words) {
  return _ranges::accumulate(words, 0, [](int accum, const string& str) {
    return accum + (str == "word");
  });
}

// clang-format off
using Complex = complex<double>;
struct Polar {
  Complex cartesian() const { return polar(abs, arg); }
  double abs = 0.0;
  double arg = 0.0;
};

// 複素数の vector をとり複素数を返す
// 返す複素数の絶対値は vector の中で絶対値が最大の複素数の絶対値
// 偏角は vector の複素数の偏角の総和
Complex rotate_greatest_radius(const vector<Complex>& v) {
  return _ranges::accumulate(v, Polar{}, [](Polar p, Complex c) {
    return Polar{
      .abs = max(p.abs, abs(c)),
      .arg = p.arg + arg(c),
    };
  }).cartesian();
}

string reverse_str(const string& str) {
  return _ranges::accumulate(str, string{}, [](auto&& str, char c) {
    return c + std::move(str);
  });
}

int main() {
  vector<Complex> v{
    {1., 1.},
    {2., 2.},
    {3., 3.},
  };
  std::cout << rotate_greatest_radius(v) << std::endl;
  std::cout << reverse_str("string"s) << std::endl;
}
