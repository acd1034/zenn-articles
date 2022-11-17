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

個人的に C++23 最大の機能は、[P0847 Deducing `this`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html) にて提案された **明示的オブジェクトパラメタの導入** だと感じています。そこで本記事では [P0847 Deducing `this`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html) と、これに隣接する 2 つの論文で導入された機能を紹介します。

## 明示的オブジェクトパラメタとは?

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
  void f1(this const X& self);
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
  // メンバ関数と同じ構文で呼び出すことができる。f2, f3, f4 も同様
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
