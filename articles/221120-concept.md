---
title: コンセプト
emoji: ☯️
type: tech
topics: [cpp, cpp23]
published: false
---

## はじめに

皆さんコンセプトは好きですか? 私は好きです。C++20でコンセプトが導入されたおかげで、型制約が簡単に書けるようになりましたね。本記事はそんなコンセプトに狂喜乱舞した筆者が個人的に気に入っているコンセプトについて紹介する記事です。なお、予め断っておきますが、**中身はあまりありません**。

## コンセプトが書ける場所

最もオーソドックスな書き方は、`template <...>` の下に requires 節を書く書き方です。

```cpp
template <class R, class Pred>
requires std::ranges::input_range<R> and 
         std::predicate<Pred, std::ranges::range_reference_t<R>>
constexpr bool all_of(R&& r, Pred pred);
```

`class` あるいは `typename` の代わりに、コンセプトを書くこともできます。かなりすっきりしましたね。

```cpp
template <std::ranges::input_range R, 
          std::predicate<std::ranges::range_reference_t<R>> Pred>
constexpr bool all_of(R&& r, Pred pred);
```

さらに、C++20で導入された、[auto による関数宣言](https://cpprefjp.github.io/lang/cpp20/function_templates_with_auto_parameters.html) と組み合わせることで、以下のように書き下すこともできます。

```cpp
constexpr bool all_of(std::ranges::input_range auto&& r,
                      std::predicate<std::ranges::range_reference_t<R>> auto pred);
```

こっちの方が格好よくないですか!? (個人の感想です) 筆者はこの書き方が気に入ったので、以下ではこの書き方を多用します。
なお、この書き方はパラメータの完全転送が面倒であるという欠点を持っています。

```cpp
constexpr bool all_of(std::ranges::input_range auto&& r,
                      std::predicate<std::ranges::range_reference_t<R>> auto pred) {
  return all_of_imp(std::forward<decltype(r)>(r), std::move(pred));
  //                ^~~~~~~~~~~~~~~~~~~~~~~~~~~~ いちいち decltype(r) と書くのが面倒...
}
```

そのため以下のようなマクロを使用します。

```cpp
#define FWD(x) std::forward<decltype(x)>(x)

constexpr bool all_of(std::ranges::input_range auto&& r,
                      std::predicate<std::ranges::range_reference_t<R>> auto pred) {
  return all_of_imp(FWD(r), std::move(pred));
  //                ^~~~~~ 簡単になった!
}
```

余談ですが、`std::forward<decltype(x)>(x)` の効果は `static_cast<decltype(x)&&>(x)` と等価です。ただし、`std::forward` は右辺値を左辺値で転送しようとした場合にハードエラーとなります。優しい仕様ですね。
ここからコンセプトの紹介に移ります。

## `anything`

1つの型に対するコンセプトであり、常に真となります。ネタです。

```cpp
template <class> concept anything = true;
```

何の制約も持たないパラメタにつけることで、意味ありげな雰囲気を醸し出すことができます。なんかかっこいいですね。

```cpp
inline constexpr auto identity = [](anything auto&& x) -> decltype(x)&& {
  return static_cast<decltype(x)&&>(x);
};
```

なお同様に、常に偽となるコンセプトも定義することができます。こちらは救いようがありません。

```cpp
template <class> concept nothing = false;

template <class T>
std::add_rvalue_reference_t<T> declval() noexcept {
  static_assert(nothing<T>, "declval not allowed in an evaluated context");
}
```

## `similar_to`

2つの型に対するコンセプトであり、cv・参照修飾を除いた型が一致する場合に真となります。

```cpp
template <class T, class U>
concept similar_to = std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;
```

こちらはその簡単さとは裏腹に非常に重宝します。1つ目の例は、型が確定しているパラメータに使用して、パラメータを完全転送する例:

```cpp
constexpr auto do_something(similar_to<std::string> auto&& str) {
  
}
```

2つ目は、2つ目のパラメタが1つ目のパラメタとcv・参照修飾を除いて同じ型を取る例:

```cpp
template <anything T, similar_to<T> U>
constexpr auto do_something(T&& t, U&& u) {
  
}
```

## `rvalue`

型が **左辺値でない** 場合に真となるコンセプト。値渡しや forwarding reference で受け取った右辺値に対応するために、このような定義としています。

```cpp
template <class T>
concept rvalue = not std::is_lvalue_reference_v<T>;
```

このコンセプトは、任意の型を受け取りたいが、完全転送せずに左辺値と右辺値で操作を書き分けたい場合に使用することができます。C++ の完全転送の仕組みが完璧すぎて、少し使い所が難しいです。以下は左辺値の range と右辺値の range で操作を書き分ける例:

```cpp
template <class T>
concept rvalue_range = rvalue<T> and std::ranges::range<T>;

constexpr auto accumulate(std::ranges::range auto& r, auto init) {
  return std::ranges::fold_left(r, std::move(init), std::plus{});
}

constexpr auto accumulate(rvalue_range auto&& r, auto init) {
  return std::ranges::fold_left(r | std::views::as_rvalue, std::move(init), std::plus{});
}
```
