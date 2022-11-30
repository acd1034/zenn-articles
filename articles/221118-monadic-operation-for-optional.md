---
title: std::optional のモナド的操作
emoji: ☯️
type: tech
topics: [cpp, cpp23]
published: false
---

:::message
この記事は [C++ Advent Calendar 2022](https://qiita.com/advent-calendar/2022/cxx) の n 日目の記事です。
:::

- **概要**: 本記事では C++23 で `std::optional` に導入された、モナド的操作の機能と使用例を紹介しています。さらに他の言語との比較から、新たなエラー伝播の仕組みについても言及しています

## はじめに

[メンバ関数の新しい書き方、あるいは Deducing this](https://zenn.dev/acd1034/articles/221117-deducing-this) において、明示的オブジェクトパラメタと `std::forward_like` を用いることで、メンバ変数の転送を簡潔に記述できることを説明しました。その恩恵を受ける STL のクラスの 1 つ (そして [P0847 Deducing `this`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html) でも度々取り上げられた例) として、`std::optional` が挙げられます。これは、`std::optional` が値を所有するクラスであり、クラスオブジェクトの値カテゴリで所有する値を転送する場面が頻出するためです。

一方、C++23 で `std::optional` に新たなメソッドが追加されました[^monadic-op]。それは `transform`, `and_then`, `or_else` の 3 種類であり、まとめてモナド的操作 (_monadic operation_) と呼ばれています。これらのメソッドも、明示的オブジェクトパラメタと `std::forward_like` を用いることで、簡潔に記述することができます。

[^monadic-op]: [P0798R8 Monadic operations for `std::optional`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0798r8.html)

以下では `std::optional` に追加されたモナド的操作の紹介も兼ねて、明示的オブジェクトパラメタを用いたこれらのメソッドの実装例を紹介したいと思います。

### `transform`

```cpp
template <class Self, class F>
constexpr auto transform(this Self&& self, F&& f);
```

`transform` は呼び出し可能なオブジェクト `f` を受け取り、無効値はそのまま、有効値は `f` を適用した値をもつ有効値に変換するメソッドです。
**▼ 使用例**

```cpp
int main() {
  std::optional o(std::string("hello"));
  std::cout << o.transform(&std::string::size).value() << std::endl; // 5 を出力
}
```

**▼ 実装例**

```cpp
// @@ struct optional {
  template <class Self, std::invocable<__forward_like_t<Self, T>> F>
  constexpr auto transform(this Self&& self, F&& f)
    -> optional<std::remove_cvref_t<std::invoke_result_t<F&&, __forward_like_t<Self, T>>>> {
    if (self)
      return std::invoke(std::forward<F>(f), std::forward_like<Self>(*self));
    else
      return std::nullopt;
  }
```

ただし、`__forward_like_t` は `std::forward_like` の戻り値の型を表すエイリアステンプレートです (STL にないことを表すために、先頭に `__` を付けています)。

```cpp
template <class T, class U>
using __forward_like_t = decltype(std::forward_like<T>(std::declval<U>()));
```

### `and_then`

```cpp
template <class Self, class F>
constexpr auto and_then(this Self&& self, F&& f);
```

`and_then` も `transform` と同様に、呼び出し可能なオブジェクト `f` で有効値を変換するメソッドです。ただし、`and_then` で受け取る呼び出し可能オブジェクト `f` は、`optional` を返す必要があります。これによって、有効値を保持する `optional` に対して、失敗するかもしれない操作を行うことができます。
**▼ 使用例**

```cpp
int main() {
  constexpr auto head = [](const std::string& str) -> std::optional<char> {
    if (str.empty())
      return std::nullopt;
    else
      return str.front();
  };
  std::optional o(std::string("hello"));
  std::cout << o.and_then(head).value() << std::endl; // h を出力
}
```

**▼ 実装例**

```cpp
// @@ struct optional {
  template <class Self, std::invocable<__forward_like_t<Self, T>> F>
  requires __is_optional_v<
    std::remove_cvref_t<std::invoke_result_t<F&&, __forward_like_t<Self, T>>>>
  constexpr auto and_then(this Self&& self, F&& f)
    -> decltype(std::invoke(std::forward<F>(f), std::forward_like<Self>(*self))) {
    if (self)
      return std::invoke(std::forward<F>(f), std::forward_like<Self>(*self));
    else
      return std::nullopt;
  }
```

ただし、`__is_optional_v` は `optional` であるか否かを表す変数テンプレートです。

```cpp
template <class T>
inline constexpr bool __is_optional_v = false;
template <class T>
inline constexpr bool __is_optional_v<optional<T>> = true;
```

### `or_else`

```cpp
template <class Self, class F>
constexpr auto or_else(this Self&& self, F&& f);
```

`transform`, `and_then` が有効値を変換するメソッドであるのに対し、`or_else` は無効値を変換するメソッドです。`or_else` は引数を取らず、`optional<T>` を返す呼び出し可能オブジェクト `f` を受け取ります。そして有効値はそのまま、無効値は `f()` に変換します。
**▼ 使用例**

```cpp
int main() {
  std::optional<std::string> o = std::nullopt;
  // 無効値は空の文字列に変換して続行
  std::cout << o.or_else([] { return std::optional(std::string()); }).value() << std::endl;
}
```

**▼ 実装例**

```cpp
// @@ struct optional {
  template <class Self, std::invocable F>
  requires std::same_as<std::remove_cvref_t<std::invoke_result_t<F&&>>, optional<T>>
  constexpr optional<T> or_else(this Self&& self, F&& f) {
    if (self)
      return std::forward<Self>(self);
    else
      return std::invoke(std::forward<F>(f));
  }
```

- [Compiler Explorer での実行例](https://godbolt.org/z/qKn53KczK)

## モナド的操作の使用例

モナド的操作の使用例として、空白区切りの二項演算を表す文字列を受け取り、計算結果の値を返す関数 `parse_expr` を書いてみます。ここで `parse` は数字からなる文字列を受け取り、その数値を返す関数です。この関数は `optional` を返すことに留意して、`and_then` を用いて操作の継続を表します。

```cpp
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
  return parse<int32_t>(string_view(toks[0]))
    .and_then([&](int32_t n) {
      return parse<int32_t>(string_view(toks[2]))
        .and_then([&](int32_t m) -> optional<int32_t> {
          switch (toks[1][0]) {
            case '+': return n + m;
            case '-': return n - m;
            case '*': return n * m;
            case '/': return n / m;
            default:  return nullopt;
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
```

- [Compiler Explorer での実行例](https://godbolt.org/z/4Kv8hjr9v)

### 括弧...多くない?

失敗し得る操作を記述している部分だけ抜き出してみます。

```cpp
  return parse<int32_t>(string_view(toks[0]))
    .and_then([&](int32_t n) {
      return parse<int32_t>(string_view(toks[2]))
        .and_then([&](int32_t m) -> optional<int32_t> {
          switch (toks[1][0]) {
            case '+': return n + m;
            case '-': return n - m;
            case '*': return n * m;
            case '/': return n / m;
            default:  return nullopt;
          }
        });
    });
```

確かに多いです (笑)。`and_then` を一度書くごとに、`and_then` のメンバ関数呼び出しで丸括弧 `()` が 1 つ、ラムダ式で波括弧 `{}` が 1 つ、計 2 つの括弧がネストされます。そのためにコードが少し読みづらくなっています。

### 他の言語の場合

一方、他の言語で `parse_expr` を記述した場合は、どうなるのでしょうか。ここでは Haskell と Rust の実装例を見てみたいと思います。
**▼ Haskell の場合**

```hs
import Control.Exception
import Text.Read

parseExpr :: String -> Maybe Int
parseExpr s = do
  let [tok0, tok1, tok2] = words s
  n <- readMaybe tok0 :: Maybe Int
  m <- readMaybe tok2 :: Maybe Int
  case tok1 of
    "+" -> Just $ n + m
    "-" -> Just $ n - m
    "*" -> Just $ n * m
    "/" -> Just $ n `div` m
    _   -> Nothing

main :: IO ()
main = do
  assert (parseExpr "1 + 2" == (Just $ 1 + 2)) $
    assert (parseExpr "478 - 234" == (Just $ 478 - 234)) $
    assert (parseExpr "15 * 56" == (Just $ 15 * 56)) $
    assert (parseExpr "98 / 12" == (Just $ 98 `div` 12)) $
    return ()
```

**▼ Rust の場合**

```rust
fn parse_expr(s: &str) -> Option<i32> {
  let toks: Vec<_> = s.split_whitespace().collect();
  let n: i32 = toks[0].parse().ok()?;
  let m: i32 = toks[2].parse().ok()?;
  match toks[1] {
    "+" => Some(n + m),
    "-" => Some(n - m),
    "*" => Some(n * m),
    "/" => Some(n / m),
    _   => None,
  }
}

fn main() {
  assert!(parse_expr("1 + 2") == Some(1 + 2));
  assert!(parse_expr("478 - 234") == Some(478 - 234));
  assert!(parse_expr("15 * 56") == Some(15 * 56));
  assert!(parse_expr("98 / 12") == Some(98 / 12));
}
```

わざと C++ の例と同じロジックで記述し、変数名や `optional` (Haskell では `Maybe`, Rust では `Option`) の使用箇所も揃えてあります。Haskell や Rust の方が随分すっきりした見た目に見えるのではないでしょうか。実は Haskell や Rust の簡潔な見た目の裏側には、強力な言語機能があります。

Haskell では、**do 記法** (_do notation_) と呼ばれる構文を使用することができます。これは、モナド則をみたすオブジェクトに対して実行する `and_then` (Haskell では中置記法の二項演算子 `>>=`) で記述される連続操作を、簡潔に書き下すための糖衣構文です。

```hs
-- Haskell ではこのコードを
mx >>= (\x -> f x >>= (\y -> g x y))
-- こう書くことができる
do { x <- mx; y <- f x; g x y }
```

Rust は言語としてモナドをサポートしている訳ではありませんが、**エラー伝播演算子 `?`** (_error propagation operator_) を使用することができます。これは、オブジェクトがある条件をみたす場合に操作を続行し、そうでない場合に中断して早期リターンするための演算子です。この演算子はまさしく Haskell の do 記法を模倣したものといえると思います。

```rust
fn parse_expr(o: Option<_>) -> Option<_> {
  // このコードは
  o?
  // こう展開されるイメージ (厳密ではない)
  match o {
    Some(v) => v,
    None => return None,
  }
}
```

Haskell や Rust ではこれらの言語機能が括弧のネストを減らすことに貢献しています。

:::details 余談: do 記法やエラー伝播演算子を使用せずに書くこともできます
▼ Haskell の場合

```hs
import Control.Exception
import Text.Read

parseExpr :: String -> Maybe Int
parseExpr s =
  let [tok0, tok1, tok2] = words s in
    (readMaybe tok0 :: Maybe Int) >>= \n ->
      (readMaybe tok2 :: Maybe Int) >>= \m ->
        case tok1 of
          "+" -> Just $ n + m
          "-" -> Just $ n - m
          "*" -> Just $ n * m
          "/" -> Just $ n `div` m
          _ -> Nothing
```

▼ Rust の場合

```rust
fn parse_expr(s: &str) -> Option<i32> {
  let toks: Vec<_> = s.split_whitespace().collect();
  toks[0].parse().ok().and_then(|n: i32| {
    toks[2].parse().ok().and_then(|m: i32| match toks[1] {
      "+" => Some(n + m),
      "-" => Some(n - m),
      "*" => Some(n * m),
      "/" => Some(n / m),
      _ => None,
    })
  })
}
```

:::

### C++ でも同じ書き方はできないの?

現時点ではできません。しかし、Rust と同様のエラー伝播演算子の導入が提案されています。

- [P2561 An error propagation operator](https://wg21.link/p2561)

C++23 策定終了まで残りわずかですが、言語機能を設計する作業グループでは、この提案にリソースを投入することに合意がとれているようです[^consensus]。もしかしたら将来、C++でもこのような記法が実現されるかもしれません。

```cpp
// 将来の書き方??
constexpr auto parse_expr(string_view sv) -> optional<int32_t> {
  const auto toks = sv | views::split(' ') | ranges::to<vector>();
  auto n = parse<int32_t>(string_view(toks[0]))??;
  auto m = parse<int32_t>(string_view(toks[2]))??;
  switch (toks[1][0]) {
    case '+': return n + m;
    case '-': return n - m;
    case '*': return n * m;
    case '/': return n / m;
    default:  return nullopt;
  }
}
```

[^consensus]: https://github.com/cplusplus/papers/issues/1276#issuecomment-1310804047

## おわりに

C++23 で導入された、`std::optional` のモナド的操作の実装例と使用例を紹介しました。本記事では触れませんでしたが、C++23 では正常値と異常値の両方を表現できる、`std::expected` の導入も決定されています。`std::expected` でも `std::optional` と同様のモナド的操作が提供される予定です。

さらに、操作の継続・中断を実現できる、エラー伝播演算子の提案についても軽く触れました。もしこの提案が C++23 に向けて採択されれば、C++20 から C++23 への移行でエラー処理の方法が革命的に変わります。そうなれば C++23 は、メジャーアップデートと呼んで差し支えない影響を持ちそうです。

## 主要な参考文献

- [P0798R8 Monadic operations for `std::optional`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0798r8.html)
- [P2561R1 An error propagation operator](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2561r1.html)
