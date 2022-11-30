#include <cassert>
#include <charconv>
#include <concepts>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>
using namespace std; // 見やすさのため

template <integral Int>
constexpr auto parse(string_view sv) -> optional<Int> {
  Int n{};
  auto [ptr, ec] = from_chars(sv.data(), sv.data() + sv.size(), n);
  if (ec == errc{} and ptr == sv.data() + sv.size())
    return n;
  else
    return nullopt;
}

constexpr auto parse_expr(string_view sv) {
  const auto toks = sv | views::split(' ') | ranges::to<vector>();
  return parse<int32_t>(string_view(toks[0])) //
    .and_then([&](int32_t n) {
      return parse<int32_t>(string_view(toks[2]))
        .and_then([&](int32_t m) -> optional<int32_t> {
          switch (toks[1][0]) {
          case '+':
            return n + m;
          case '-':
            return n - m;
          case '*':
            return n * m;
          case '/':
            return n / m;
          default:
            return nullopt;
          }
        });
    });
}

int main() {
  assert(parse_expr("1 + 2"sv) == optional(1 + 2));
  assert(parse_expr("478 - 234"sv) == optional(478 - 234));
  assert(parse_expr("15 * 56"sv) == optional(15 * 56));
  assert(parse_expr("98 / 12"sv) == optional(98 / 12));
}
