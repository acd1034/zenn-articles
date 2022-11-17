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
  - [P0798R3 Monadic operations for `std::optional`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0798r3.html)

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

[^monadic-op]: [P0798R3 Monadic operations for `std::optional`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0798r3.html)

以下では `std::optional` に追加されたモナド的操作の紹介も兼ねて、これらのメソッドの実装例を紹介したいと思います。
