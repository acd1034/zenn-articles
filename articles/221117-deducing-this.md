---
title: 【C++23】クラスオブジェクトパラメタの明示的宣言
emoji: 🧑‍🏫
type: tech
topics: [cpp, cpp23]
published: false
---

:::message
この記事は [C++ Advent Calendar 2022](https://qiita.com/advent-calendar/2022/cxx) の 1 日目の記事です。
:::

- **概要**: 本記事では C++23 に向けて採択された以下の論文の機能を紹介しています
  - [P0847R7 Deducing `this`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html)
  - [P2445R1 `std::forward_like`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2445r1.pdf)
  - [P0798R8 Monadic operations for `std::optional`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0798r8.html)

## はじめに

2022 年も年の瀬を迎え、C++23 の発行があと 1 年に迫ってきました。2022 年 11 月の Kona 会議は無事終了し、順調に進めば 2023 年 2 月の Issaquah 会議で策定が完了する予定です[^kona]。そろそろ C++23 の新機能が気になってきた人も多いのではないでしょうか。

[^kona]: [Trip report: Autumn ISO C++ standards meeting (Kona) - Sutter’s Mill](https://herbsutter.com/2022/11/12/trip-report-autumn-iso-c-standards-meeting-kona/)

個人的に C++23 最大の機能は、[P0847 Deducing `this`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html) にて提案された **明示的オブジェクトパラメタの導入** だと感じています[^my-wish]。そこで本記事では [P0847 Deducing `this`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html) と、これに隣接する 2 つの論文で導入された機能を紹介します。

[^my-wish]: 今後すべてのメンバ関数を明示的オブジェクトパラメタを用いて宣言したいくらい

## 明示的オブジェクトパラメタとは

オブジェクトパラメタ (_object parameter_) とは、クラスのメンバ関数内で、クラスオブジェクト自身を指すパラメタのことです。C++では、クラスオブジェクト自身のポインタを指すキーワードとして、`this` を使用することができます。これはメンバ関数内で暗黙的に宣言されるため、暗黙的オブジェクトパラメタ (_implicit object parameter_) と呼ばれます。

一方、明示的オブジェクトパラメタ (_explicit object parameter_) を宣言することは、従来の C++ではできませんでした。これを実現したのが、C++23 に向けて採択された論文、[P0847 Deducing `this`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html) です。

```cpp
struct X {
  void f() const& {}

  void g1() const& {
    this->f();
//  ^~~~ 暗黙的オブジェクトパラメタ
  }

  void g2(this const X& self) {
    self.f();
//  ^~~~ 明示的オブジェクトパラメタ
  }
};
```

### どうやって明示的オブジェクトパラメタを宣言できるか?

明示的オブジェクトパラメタを導入するために、メンバ関数を宣言する新たな構文が追加されました。その構文では、メンバ関数の最初のパラメタの先頭に、`this` キーワードを付けることができます。

```cpp
struct X {
  // const X&　型のクラスオブジェクトを受け取る
  void f1(this const X& self) {}
  // X&&　型のクラスオブジェクトを受け取る
  void f1(this X&& self);

  // クラスオブジェクトの型をテンプレートパラメタにすることもできる
  template <class Self>
  void f2(this const Self& self);

  // クラスオブジェクトを forwarding reference で受け取ることもできる
  template <class Self>
  void f3(this Self&& self);

  // テンプレートパラメタを auto にすることもできる
  void f4(this auto&& self);
};

void foo(X& x) {
  // 従来の非静的メンバ関数と同じ構文で呼び出すことができる。f2, f3, f4 も同様
  x.f1();
}
```

この構文はラムダ式でも使用することができます。

```cpp
void f() {
  int captured = 0;
  // ラムダ式でも明示的オブジェクトパラメタを宣言できる
  auto lambda = [captured](this auto&& self) -> decltype(auto) {
    return std::forward_like<decltype(self)>(captured);
  };
  // 従来と同じ構文で呼び出すことができる
  lambda();
}
```

オブジェクトパラメタのパラメタ名 (上記の例の `self`) は、`self` 以外でも構いません。しかし、他言語の慣例に則って `self` とするのが一般的なようです。

### 注意点

前項で、メンバ関数を宣言する新たな構文が追加されたことを説明しました。これは従来の非静的メンバ関数の宣言と、いくつか異なる点があります。

- **`static`・`virtual` を指定することはできず、cv・参照修飾することもできない**

  ```cpp
  struct B {
    static  void f1(this B const&);  // error
    virtual void f2(this B const&);  // error
    virtual void f3() const&;        // OK
    void f4(this B) const&;          // error
  };

  struct D : B {
    void f3() const& override;       // OK
    void f3(this D const&) override; // error
  };
  ```

- オーバーロードで暗黙的オブジェクトパラメタと明示的オブジェクトパラメタを混在させることは許可されている。しかし、**オーバーロード解決が不可能な混在は許可されていない**

  ```cpp
  struct X {
    void f1(this X&);
    void f1() &&;           // OK: another overload
    void f2(this const X&);
    void f2() const&;       // error: redeclaration
    void f3();
    void f3(this X&);       // error: redeclaration
  };
  ```

- 明示的オブジェクトパラメタを宣言した場合、**暗黙的オブジェクトパラメタを使用することはできない**。すなわち、**`this` や非修飾メンバアクセスは使用できない**

  ```cpp
  struct X {
    int i = 0;

    int f1(this auto&&) { return this->i; }     // error
    int f2(this auto&&) { return i; }           // error
    int f3(this auto&& self) { return self.i; } // OK
  };
  ```

- クラスを継承した場合、**基底クラスで現れる明示的オブジェクトパラメタのテンプレート型は、派生クラスの型で推定される**。その結果、**派生クラスのメンバが基底クラスのメンバを隠蔽する現象が生じる**

  ```cpp
  struct B {
    int i = 0;

    int f1(this const B& self) { return self.i; }
    int f2(this const auto& self) { return self.i; }
  };

  struct D : B {
    int i = 42; // B::i を隠蔽する
  };

  void foo(B& b, D& d) {
    assert(b.f1() == 0);  // 0 を返す
    assert(b.f2() == 0);  // 0 を返す
    assert(d.f1() == 0);  // 0 を返す
    assert(d.f2() == 42); // self は D の参照で推定されるため、42 を返す
  }
  ```

  この仕様は武器にもなりますが、意図せぬ動作を引き起こす凶器にもなり得ます。

<!-- TODO: メンバ関数ポインタの型について -->

### メリット

明示的オブジェクトパラメタを forwarding reference で受け取ることで、クラスオブジェクトの cv・参照修飾を推定することができます。これによって、メンバ関数内でクラスオブジェクトを完全転送することができるようになります。

```cpp
struct negative {
  constexpr bool operator()(int x) const { return x < 0; }

  template <class Self>
  constexpr auto negate(this Self&& self) {
    return std::not_fn(std::forward<Self>(self));
    //                 ^~~~~~~~~~~~~~~~~~~~~~~~ self を完全転送できる
  }
};

inline constexpr auto non_negative = negative{}.negate();
```

これは、次で述べる `std::forward_like` と組み合わせることで、**クラスオブジェクトの const・参照修飾でメンバ変数を転送できる** ことを意味しています。

## `std::forward_like` とは

`std::forward_like` は、第一テンプレート引数の const・参照修飾を用いて引数を転送する関数です[^forward-like]。基本的には `std::forward_like<T>(x)` のように、1 つのテンプレート引数と 1 つの引数を渡して使用します。これによって `T` の const 性をマージし `T` の値カテゴリをコピーした型で、`x` を転送することができます。

[^forward-like]: [P2445R1 `std::forward_like`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2445r1.pdf)

```cpp
void f(int& a, int& b, const int& c, const int& d) {
  {
    int&  x = std::forward_like<char&>(a);
    int&& y = std::forward_like<char>(a);
    const int&  z = std::forward_like<const char&>(b);
    const int&& w = std::forward_like<const char>(b);
  }
  {
    const int&  x = std::forward_like<char&>(c);
    const int&& y = std::forward_like<char>(c);
    const int&  z = std::forward_like<const char&>(d);
    const int&& w = std::forward_like<const char>(d);
  }
}
```

`std::forward_like` を用いることで、クラスオブジェクトの const・参照修飾に合わせてメンバ変数を正しく転送することができるようになります。

```cpp
struct X {
  int i = 0;

  template <class Self>
  decltype(auto) f(this Self&& self) {
    return std::forward_like<Self>(self.i);
  }
};

void foo(X& x) {
  int& a = x.f();             // int& を返す
  int&& b = std::move(x).f(); // int&& を返す
}
```

ここまで、明示的オブジェクトパラメタと `std::forward_like` について紹介してきました。この 2 つの機能を組み合わせることで、クラスオブジェクトの値カテゴリに依存したメンバ関数を、重複なく簡潔に記述することが可能となります。以下はもっと身近な使用例を 2 つほど紹介します。

### 例 1: 値を所有するクラスの間接参照

`std::vector`, `std::tuple`, `std::optional` のような、値を所有するクラスの値の参照を簡単に書くことができるようになります。

```cpp
template <class T>
struct single {
  T value;

  template <class Self>
  constexpr decltype(auto) operator*(this Self&& self) {
    return std::forward_like<Self>(self.value);
  }
};

template <class T>
single(T) -> single<T>;

int main() {
  int i = 0;
  single s(1);
  i = *s;            // i に s.value がコピーされる
  i = *std::move(s); // i に s.value がムーブされる
}
```

### 例 2: 完全転送 call wrapper

拙著「[`__perfect_forward` の仕組みと使い方](https://zenn.dev/acd1034/articles/509b011bdf9917)」にて llvm における完全転送 call wrapper の実装上の工夫を紹介しました。完全転送 call wrapper とは、関数呼び出しの際に、自身の状態を表す内部変数を自身の const・参照修飾で転送する call wrapper のことです。明示的オブジェクトパラメタと `std::forward_like` を利用することで、完全転送 call wrapper をさらに簡潔に書き下すことができます。ここでは C++23 向けの提案 [P2387R3 Pipe support for user-defined range adaptors](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2387r3.html) で導入された、`std::bind_back` の実装例を紹介します。

```cpp
#include <functional>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>

namespace ns {
  template <class F, class Seq, class... Bound>
  struct bind_back_t;

  template <class F, std::size_t... I, class... Bound>
  struct bind_back_t<F, std::index_sequence<I...>, Bound...> {
    F f;
    std::tuple<Bound...> bound;

    template <class Self, class... Args>
    constexpr auto operator()(this Self&& self, Args&&... args) noexcept(
      noexcept(   std::invoke(std::forward_like<Self>(self.f),
                              std::forward<Args>(args)...,
                              std::forward_like<Self>(std::get<I>(self.bound))...)))
      -> decltype(std::invoke(std::forward_like<Self>(self.f),
                              std::forward<Args>(args)...,
                              std::forward_like<Self>(std::get<I>(self.bound))...)) {
      return      std::invoke(std::forward_like<Self>(self.f),
                              std::forward<Args>(args)...,
                              std::forward_like<Self>(std::get<I>(self.bound))...);
    }
  };

  template <class F, class... Args>
  requires std::is_constructible_v<std::decay_t<F>, F> and
           std::is_move_constructible_v<std::decay_t<F>> and
           (std::is_constructible_v<std::decay_t<Args>, Args> and ...) and
           (std::is_move_constructible_v<std::decay_t<Args>> and ...)
  constexpr bind_back_t<std::decay_t<F>,
                        std::index_sequence_for<Args...>,
                        std::decay_t<Args>...>
  bind_back(F&& f, Args&&... args) {
    return {std::forward<F>(f), {std::forward<Args>(args)...}};
  }
}

int main() {
  constexpr auto minus_one = ns::bind_back(std::minus{}, 1);
  std::cout << minus_one(42) << std::endl; // 41 を出力
}
```

- [Compiler Explorer での実行例](https://godbolt.org/z/zE51hGa46)

## `std::optional` のモナド的操作

明示的オブジェクトパラメタと `std::forward_like` を用いることで、メンバ関数を簡潔に記述できることを説明しました。その恩恵を受ける STL のクラスの 1 つ (そして [P0847 Deducing `this`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html) でも度々取り上げられた例) として、`std::optional` が挙げられます。これは、`std::optional` が値を所有するクラスであり、クラスオブジェクトの値カテゴリで所有する値を転送する場面が頻出するためです。

一方、C++23 で `std::optional` に新たなメソッドが追加されました[^monadic-op]。それは `transform`, `and_then`, `or_else` の 3 種類であり、まとめてモナド的操作 (_monadic operation_) と呼ばれています。これらのメソッドも、明示的オブジェクトパラメタと `std::forward_like` を用いることで、簡潔に記述することができます。

[^monadic-op]: [P0798R8 Monadic operations for `std::optional`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0798r8.html)

以下では `std::optional` に追加されたモナド的操作の紹介も兼ねて、これらのメソッドの実装例を紹介したいと思います。

### `transform`

```cpp
template <class Self, class F>
constexpr auto transform(this Self&& self, F&& f);
```

`transform` は呼び出し可能なオブジェクト `f` を受け取り、無効値はそのまま、有効値は `f` を適用した値をもつ有効値に変換するメソッドです。
▼ 使用例

```cpp
int main() {
  std::optional o(std::string("hello"));
  std::cout << o.transform(&std::string::size).value() << std::endl; // 5 を出力
}
```

▼ 実装例

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
▼ 使用例

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

▼ 実装例

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
▼ 使用例

```cpp
int main() {
  std::optional<std::string> o = std::nullopt;
  // 無効値は空の文字列に変換して続行
  std::cout << o.or_else([] { return std::optional(std::string()); }).value() << std::endl;
}
```

▼ 実装例

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

モナド的操作の使用例として、空白区切りの二項演算を表す文字列を受け取り、計算結果の値を返す関数 `read_expr` を書いてみます。ここで `parse` は数字からなる文字列を受け取り、その数値を返す関数です。この関数は `optional` を返すことに留意して、`and_then` を用いて操作の継続を表します。

```cpp
#include <cassert>
#include <charconv>
#include <concepts>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>

template <std::integral Int>
constexpr auto parse(std::string_view sv) -> std::optional<Int> {
  Int n{};
  auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), n);
  if (ec == std::errc{} and ptr == sv.data() + sv.size())
    return n;
  else
    return std::nullopt;
}

constexpr auto read_expr(std::string_view sv) {
  const auto toks =
    sv | std::views::split(' ') | std::ranges::to<std::vector>();
  return parse<std::int32_t>(std::string_view(toks[0]))
    .and_then([&](std::int32_t n) {
      return parse<std::int32_t>(std::string_view(toks[2]))
        .and_then([&](std::int32_t m) -> std::optional<std::int32_t> {
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
            return std::nullopt;
          }
        });
    });
}

int main() {
  using namespace std::string_view_literals;
  assert(read_expr("1 + 2"sv) == std::optional(1 + 2));
  assert(read_expr("478 - 234"sv) == std::optional(478 - 234));
  assert(read_expr("15 * 56"sv) == std::optional(15 * 56));
  assert(read_expr("98 / 12"sv) == std::optional(98 / 12));
}
```

- [Compiler Explorer での実行例](https://godbolt.org/z/q1j1jsGoh)

### 括弧...多くない?

失敗し得る操作を記述している部分だけ抜き出してみます。

```cpp
  return parse<std::int32_t>(std::string_view(toks[0]))
    .and_then([&](std::int32_t n) {
      return parse<std::int32_t>(std::string_view(toks[2]))
        .and_then([&](std::int32_t m) -> std::optional<std::int32_t> {
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
            return std::nullopt;
          }
        });
    });
```

確かに多いです (笑)。`and_then` を一度書くごとに、`and_then` のメンバ関数呼び出しで丸括弧 `()` が 1 つ、ラムダ式で波括弧 `{}` が 1 つ、計 2 つの括弧がネストされます。そのためにコードが少し読みづらくなっています。
