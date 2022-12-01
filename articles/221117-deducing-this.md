---
title: メンバ関数の新しい書き方、あるいは Deducing this
emoji: 👨‍👩‍👦
type: tech
topics: [cpp, cpp23]
published: true
---

:::message
この記事は [C++ Advent Calendar 2022](https://qiita.com/advent-calendar/2022/cxx) の 1 日目の記事です。
:::

- **概要**: C++23 でクラスのメンバ関数の書き方が拡張された結果、明示的オブジェクトパラメタを使用することができるようになりました。本記事ではその機能の提案文書 [P0847R7 Deducing `this`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html) の内容と、新たな機能の使い方について紹介しています

## はじめに

2022 年も年の瀬を迎え、C++23 の発行があと 1 年に迫ってきました。2022 年 11 月の Kona 会議は無事終了し、順調に進めば 2023 年 2 月の Issaquah 会議で策定が完了する予定です[^kona]。そろそろ C++23 の新機能が気になってきた人も多いのではないでしょうか。

[^kona]: [Trip report: Autumn ISO C++ standards meeting (Kona) - Sutter’s Mill](https://herbsutter.com/2022/11/12/trip-report-autumn-iso-c-standards-meeting-kona/)

個人的に C++23 最大の機能は、[P0847 Deducing `this`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html) にて提案された、**明示的オブジェクトパラメタの導入** だと感じています[^my-wish]。そこで本記事では新たに追加された機能の内容と、その使い方を紹介します。

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

### どうすれば明示的オブジェクトパラメタを宣言できる?

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

`std::forward_like` は、C++23 に向けて採択された提案 [P2445R1 `std::forward_like`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2445r1.pdf) において導入されました。これは、第一テンプレート引数の const・参照修飾を用いて引数を転送する関数です。基本的には `std::forward_like<T>(x)` のように、1 つのテンプレート引数と 1 つの引数を渡して使用します。これによって `T` の const 性をマージし `T` の値カテゴリをコピーした型で、`x` を転送することができます。

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

`std::vector`, `std::tuple`, `std::optional` のような、値を所有するクラスの値の参照を簡単に書くことができるようになります。ここでは簡易的な `tuple` の実装を紹介します。

```cpp
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
  tuple_leaf<I, T> at_index(const tuple_leaf<I, T>&); // undefined

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
                           t.template get<2>()); // (1, 3.14, "hello") を出力
}
```

- [Compiler Explorer での実行例](https://godbolt.org/z/z5o3coMb6)

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
} // namespace ns

int main() {
  constexpr auto minus_one = ns::bind_back(std::minus{}, 1);
  std::cout << minus_one(42) << std::endl; // 41 を出力
}
```

- [Compiler Explorer での実行例](https://godbolt.org/z/zE51hGa46)

## おわりに

本記事では、メンバ関数の新たな書き方が導入され、明示的オブジェクトパラメタが使用できるようになったこと、およびこれと`std::forward_like` を用いることで、メンバ変数の転送を簡潔に記述できるようになったことを紹介しました。本記事では紹介できませんでしたが、新たな言語機能が導入されたことで、自己再帰ラムダや CRT なしの CRTP など、新たなイディオムが開発 (あるいは発見) されています。今回の拡張が新たな可能性をもたらしてくれることは間違いなさそうです。

本記事で紹介した機能は、現時点で以下の処理系で使用できます。

- Deducing `this`: [Visual Studio 2022 17.2 以上](https://learn.microsoft.com/en-us/visualstudio/releases/2022/release-notes-v17.2#whats-new-in-visual-studio-2022-version-1720)
- `std::forward_like`: [Visual Studio 2022 17.4 以上](https://github.com/microsoft/STL/wiki/Changelog#vs-2022-174)

最新の MSVC は、[Compiler Explorer](https://godbolt.org/z/znaj6Waz6) で使用することもできます。試してみてください。

<!-- TODO: 本記事の続きにあたる std::optional のモナド的操作 (後日公開予定) では、明示的オブジェクトパラメタを使用したメソッドの実装例を紹介しています。あわせて読んでみてください。 -->

## 参考文献

- [P0847R7 Deducing `this`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html)
- [P2445R1 `std::forward_like`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2445r1.pdf)
- [［C++］WG21 月次提案文書を眺める（2020 年 10 月） - 地面を見下ろす少年の足蹴にされる私](https://onihusube.hatenablog.com/entry/2020/11/02/221657#P0847R5-Deducing-this)
- [［C++］WG21 月次提案文書を眺める（2021 年 10 月） - 地面を見下ろす少年の足蹴にされる私](https://onihusube.hatenablog.com/entry/2021/11/13/193322#P2445R0-forward_like)

### さらなる文献

- [自己再帰するラムダ式 @ C++23 - yohhoy の日記](https://yohhoy.hatenadiary.jp/entry/20211025/p1)
- [Copy-on-write with Deducing this - Barry's C++ Blog](https://brevzin.github.io/c++/2022/09/23/copy-on-write/)
