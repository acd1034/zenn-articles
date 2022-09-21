---
title: view の書き方を一歩ずつ
emoji: 🪟
type: tech
topics: [cpp, cpp20, cpp23]
published: false
---

- 概要： 本記事ではインクリメンタルに view の書き方を説明しています。また具体例として `enumerate_view` の実装例を紹介しています。

## はじめに

<!-- TODO: range adaptor の導入 -->

C++20 が策定されてから早くも 3 年が経過しようとしています。C++20 で最も影響の大きかった機能は range だったのではないかと個人的には感じています。`std::views::filter` や `std::views::transform` といった range adaptor が導入されたことで、新たにメモリ確保しなくても range に操作を加えた range を作れるようになり、大変便利になりました。

```cpp
for (auto x : std::views::iota(0)
                | std::views::filter([](auto x) { return x % 2 == 0; })
                | std::views::transform([](auto x) { return x * x; })
                | std::views::take(4))
  std::cout << x << ','; // output: 0,4,16,36,
```

一方、C++20 で導入された view の種類に物足りなさを覚えた人もいるのではないでしょうか。

view には大きく分けて 2 種類あります。

- view を生み出すもの
  → `std::ranges::iota_view` など。range factory と呼ばれます
- 元となる view から新たな view を生み出すもの
  → `std::ranges::filters_view` など。range adaptor と呼ばれます

本記事では後者の range adaptor を扱います。

range adaptor を実装するには、その range adaptor で実現したい操作の他にも、元となる view の性質を受け継ぐ動作を記述する必要があります。この性質を受け継ぐための記述は range adaptor の実装の多くを占める一方、そのほとんどは使い回しのできるコードとなっています。

そこで本記事では例として `enumerate_view` の実装を追うことで、その他の range adaptor の実装にも役立てることを試みます。`enumerate_view` の使用イメージ：

```cpp
std::vector<char> v{'a', 'b', 'c'};
for (auto&& [index, value] : enumerate_view(v))
  std::cout << index << ':' << value << ','; // output: 0:a,1:b,2:c,
```

`enumerate_view` を実装するには、`enumerate_view` 本体に加え、専用のイテレータと番兵イテレータを実装する必要があります。本記事における実装の出発点は以下の通りです。

```cpp
/// @tparam View 元となる view の型
template <std::ranges::input_range View>
requires std::ranges::view<View>
struct enumerate_view {
private:
  //! 元となる view
  View base_ = View();

  struct iterator;
  struct sentinel;

public:
  constexpr enumerate_view(View base) : base_(std::move(base)) {}
};

template <std::ranges::input_range View>
requires std::ranges::view<View>
struct enumerate_view<View>::iterator {
private:
  //! 元となるイテレータの現在位置
  std::ranges::iterator_t<View> current_ = std::ranges::iterator_t<View>();
  //! 現在のインデックス
  std::size_t count_ = 0;

public:
  constexpr iterator(std::ranges::iterator_t<View> current, std::size_t count)
    : current_(std::move(current)), count_(std::move(count)) {}
};

template <std::ranges::input_range View>
requires std::ranges::view<View>
struct enumerate_view<View>::sentinel {
private:
  //! 元となる view の番兵イテレータ
  std::ranges::sentinel_t<View> end_ = std::ranges::sentinel_t<View>();

public:
  constexpr explicit sentinel(std::ranges::sentinel_t<View> end)
    : end_(std::move(end)) {}
};
```

変更点を一覧表示できるよう、節の最後にはその節の変更点の差分をリンクで示しています。コミットには簡単な単体テストも含まれます。

:::message
本記事は C++20 策定後の欠陥報告を反映しているため、C++20 をサポートしていても古いコンパイラ (処理系) では動作しない可能性があります。本記事のコードは以下のコンパイラで動作することを確認しています。

- GCC 12.1.0
- Clang 16.0.0
- Visual Studio 2019 version 16.11.14

また、以下のコンパイラで動作すると思われます。

- GCC 12.1.0 以上
- Clang 16.0.0 以上
- Visual Studio 2019 version 16.11.14 以上
- Visual Studio 2022 version 17.1 以上

:::

## `view` コンセプトに対応する

実装する view の型を `V`、`V` のイテレータの型を `I`、`V` の番兵イテレータの型を `S` とします。この時 `V` が`view` コンセプトを満たすには、以下の条件が成立する必要があります。

- `V` が [後述の条件](link?) を満たす
- `I` が `std::input_or_output_iterator` コンセプトを満たす
- `S` が `std::sentinel_for<I>` コンセプトを満たす

本節ではイテレータ、番兵イテレータ、view 本体の順に見ていきます。

### `I` を `input_or_output_iterator` に対応させる

本節では `enumerate_view<View>::iterator` に変更を加えます。
`I` が `std::input_or_output_iterator` コンセプトを満たすには、以下の条件が成立する必要があります(以下 `I` 型のオブジェクトを `i` と記述します)。

- **`I` が `std::movable` コンセプトを満たす**

  デフォルト定義されているため、特に行うことはありません。

- **`I` に `typename I::difference_type` が定義されており、その型が符号付き整数型である**
  ```cpp
  using difference_type = std::ranges::range_difference_t<View>;
  ```
- **前置インクリメント `++i` が定義されており、戻り値の型が `I&` である**
  ```cpp
  constexpr iterator& operator++() {
    ++current_;
    ++count_;
    return *this;
  }
  ```
- **後置インクリメント `i++` が定義されている**
  ```cpp
  constexpr void operator++(int) { ++*this; }
  ```
- **間接参照演算子 `*i` が定義されており、戻り値の型が参照修飾できる**
  ```cpp
  constexpr std::pair<std::size_t, std::ranges::range_reference_t<View>> //
  operator*() const {
    return {count_, *current_};
  }
  ```

[本節の差分](link?)

### `S` を `sentinel_for` に対応させる

本節では `enumerate_view<View>::sentinel` に変更を加えます。
`S` が `std::sentinel_for<I>` コンセプトを満たすには、以下の条件が成立する必要があります(以下 `S` 型のオブジェクトを `s` と記述します)。

- **`S` が `std::semiregular` コンセプトを満たす** (すなわちムーブ・コピー・デフォルト初期化可能である)

  デフォルトコンストラクタが無いため追加します。

  ```cpp
  sentinel() = default;
  ```

- **等値比較演算子 `i == s` が定義されている**

  ```cpp
  friend constexpr bool
  operator==(const iterator& x, const sentinel& y) requires
    std::sentinel_for<std::ranges::sentinel_t<View>,
                      std::ranges::iterator_t<View>> {
    return x.base() == y.end_;
  }
  ```

  :::details base() について
  実装のために `enumerate_view<View>::iterator` にメンバ関数 `base()` を追加しています。

  ```cpp
  constexpr const std::ranges::iterator_t<View>& base() const& noexcept {
    return current_;
  }
  constexpr std::ranges::iterator_t<View> base() && {
    return std::move(current_);
  }
  ```

  :::

  <!-- TODO: operator== の自動定義について説明する -->

以後 [`sized_sentinel_for`・`sized_range` に対応する](link?) まで、`enumerate_view<View>::sentinel` は触りません。

[本節の差分](link?)

### `V` を `view` コンセプトに対応させる

本節では `enumerate_view` に変更を加えます。
`V` が `std::ranges::view` コンセプトを満たすには、以下の条件が成立する必要があります。

- **`V` が `std::movable` コンセプトを満たす**

  デフォルト定義されているため、特に行うことはありません。

- **メンバ関数 `begin()` が定義されている**
  ```cpp
  constexpr iterator begin() { return {std::ranges::begin(base_), 0}; }
  ```
- **メンバ関数 `end()` が定義されている**
  ```cpp
  constexpr auto end() { return sentinel(std::ranges::end(base_)); }
  ```
- **`V` が `std::ranges::view_interface` を継承している**
  ```cpp
  struct enumerate_view : std::ranges::view_interface<enumerate_view<View>> {
  ```

また、`V` が `std::ranges::view` コンセプトを満たすためには不要ですが、慣例に倣い以下の変更を加えます。

- **`V` にデフォルトコンストラクタを追加する**
  ```cpp
  enumerate_view() requires std::default_initializable<View> = default;
  ```
- **`V` に以下の推定ガイドを追加する**
  ```cpp
  template <class Range>
  enumerate_view(Range&&) -> enumerate_view<std::views::all_t<Range>>;
  ```
  <!-- TODO: この推定ガイドの意義について説明する -->

以後 [`sized_sentinel_for`・`sized_range` に対応する](link?) まで、`enumerate_view` は触りません。

[本節の差分](link?)

## `input_iterator` に対応する

イテレータは、そのイテレータが提供する操作によって分類することができます。この分類はイテレータカテゴリと呼ばれ、イテレータカテゴリには提供する操作に基づき順序構造が定められています。C++20 時点で提供されているイテレータカテゴリとその順序は以下のようになります (`output_iterator` は省略)。

```cpp
input_iterator < forward_iterator
               < bidirectional_iterator
               < random_access_iterator
               < contiguous_iterator
```

`enumerate_view` などの range adaptor は元となる view から新たな view を生み出す操作であり、その多くは元となる view のイテレータカテゴリを受け継ぐことができます。そこで、本節から [random_access_range に対応する](link?) までに渡り、元となる view があるイテレータカテゴリを満たす場合に `enumerate_view` が同じイテレータカテゴリを満たすよう、変更を加えます。なお、イテレータカテゴリは view や番兵イテレータに依らず、イテレータの操作のみによって定まります。そのため、以後 [random_access_range に対応する](link?) までは `enumerate_view<View>::iterator` のみに変更を加えます。

`I` が `std::input_iterator` コンセプトを満たすには、以下の条件が成立する必要があります。

- **`I` が `std::input_or_output_iterator` コンセプトを満たす**

  上記で対応済みです。

- **`I` に `typename I::value_type` が定義されており、その型がオブジェクト型である**
  ```cpp
  using value_type = std::pair<std::size_t, std::ranges::range_value_t<View>>;
  ```
- **`I` に `typename I::iterator_concept` が定義されており、その型が `std::input_iterator_tag` を継承している**
  ```cpp
  using iterator_concept = std::input_iterator_tag;
  ```
- **必要に応じて非メンバ関数 `iter_move(i)` が定義されている**

  `std::ranges::iter_move` にはデフォルトの定義が存在するため、定義は必須ではありません。しかし手動で定義した方がよい場合があります。これについては [`iter_move` について](link?) で説明します。
  <!-- lvalue-referenceを保持する型(std::pair<std::size_t, T&>など)を返すイテレータは、iter_moveを定義した方がよい -->
  <!-- lvalue-referenceを保持する型 のことを proxy reference と呼ぶらしい -->
  <!-- TODO: iter_move はここで定義するか? 後ろで定義するか? -->

[本節の差分](link?)

## `forward_iterator` に対応する

`I` が `std::forward_iterator` コンセプトを満たすには、以下の条件が成立する必要があります。

- **`I` が `std::input_iterator` コンセプトを満たす**

  上記で対応済みです。

- **`I` が `std::semiregular` コンセプトを満たす**(すなわち上記(ムーブ可能)に加え、コピー・デフォルト初期化可能である)

  デフォルトコンストラクタが無いため追加します。

  ```cpp
  iterator() requires
    std::default_initializable<std::ranges::iterator_t<View>> = default;
  ```

- **`typename I::iterator_concept` が `std::forward_iterator_tag` を継承している**
  ```diff cpp
  - using iterator_concept = std::input_iterator_tag;
  + using iterator_concept =
  +   std::conditional_t<std::ranges::forward_range<View>,       std::forward_iterator_tag,
  +   /* else */                                                 std::input_iterator_tag>;
  ```
- **等値比較演算子 `==` が定義されている**
  ```cpp
  friend constexpr bool operator==(const iterator& x, const iterator& y) //
    requires std::equality_comparable<std::ranges::iterator_t<View>> {
    return x.current_ == y.current_;
  }
  ```
- **後置インクリメント `i++` の戻り値の型が `I` である**
  ```cpp
  constexpr iterator
  operator++(int) requires std::ranges::forward_range<View> {
    auto tmp = *this;
    ++*this;
    return tmp;
  }
  ```

補足ですが、後置インクリメントはほとんどの場合に上記のコードで定義することができます。そのようなコードをボイラープレートと呼びます。

[本節の差分](link?)

## `bidirectional_iterator` に対応する

`I` が `std::bidirectional_iterator` コンセプトを満たすには、以下の条件が成立する必要があります。

- **`I` が `std::forward_iterator` コンセプトを満たす**

  上記で対応済みです。

- **`typename I::iterator_concept` が `std::bidirectional_iterator_tag` を継承している**
  ```diff cpp
    using iterator_concept =
  +   std::conditional_t<std::ranges::bidirectional_range<View>, std::bidirectional_iterator_tag,
      std::conditional_t<std::ranges::forward_range<View>,       std::forward_iterator_tag,
  -   /* else */                                                 std::input_iterator_tag>;
  +   /* else */                                                 std::input_iterator_tag>>;
  ```
- **前置デクリメント `--i` が定義されており、戻り値の型が `I&` である**
  ```cpp
  constexpr iterator&
  operator--() requires std::ranges::bidirectional_range<View> {
    --current_;
    --count_;
    return *this;
  }
  ```
- **後置デクリメント `i--` が定義されており、戻り値の型が `I` である**
  ```cpp
  constexpr iterator
  operator--(int) requires std::ranges::bidirectional_range<View> {
    auto tmp = *this;
    --*this;
    return tmp;
  }
  ```

補足ですが、後置デクリメントはボイラープレートです。

[本節の差分](link?)

## `random_access_iterator` に対応する

`I` が `std::random_access_iterator` コンセプトを満たすには、以下の条件が成立する必要があります(以下では `typename I::difference_type` 型のオブジェクトを `n` と記述しています)。

- **`I` が `std::bidirectional_iterator` コンセプトを満たす**

  上記で対応済みです。

- **`typename I::iterator_concept` が `std::random_access_iterator_tag` を継承している**
  ```diff cpp
    using iterator_concept =
  +   std::conditional_t<std::ranges::random_access_range<View>, std::random_access_iterator_tag,
      std::conditional_t<std::ranges::bidirectional_range<View>, std::bidirectional_iterator_tag,
      std::conditional_t<std::ranges::forward_range<View>,       std::forward_iterator_tag,
  -   /* else */                                                 std::input_iterator_tag>>;
  +   /* else */                                                 std::input_iterator_tag>>>;
  ```
- **比較演算子 `<, >, <=, =>` が定義されている**
  ```cpp
  friend constexpr bool operator<(const iterator& x, const iterator& y) //
    requires std::ranges::random_access_range<View> {
    return x.current_ < y.current_;
  }
  friend constexpr bool operator>(const iterator& x, const iterator& y) //
    requires std::ranges::random_access_range<View> {
    return y < x;
  }
  friend constexpr bool operator<=(const iterator& x, const iterator& y) //
    requires std::ranges::random_access_range<View> {
    return not(y < x);
  }
  friend constexpr bool operator>=(const iterator& x, const iterator& y) //
    requires std::ranges::random_access_range<View> {
    return not(x < y);
  }
  ```
- **イテレータと数値の加算 `i += n`** (戻り値の型が `I&`), **`i + n, n + i`** (戻り値の型が `I`) **が定義されている**
  ```cpp
  constexpr iterator& operator+=(difference_type n) //
    requires std::ranges::random_access_range<View> {
    current_ += n;
    count_ += n;
    return *this;
  }
  friend constexpr iterator operator+(iterator x, difference_type n) //
    requires std::ranges::random_access_range<View> {
    x += n;
    return x;
  }
  friend constexpr iterator operator+(difference_type n, iterator x) //
    requires std::ranges::random_access_range<View> {
    x += n;
    return x;
  }
  ```
  <!-- TODO: RVOについて書く? -->
- **イテレータと数値の減算 `i -= n`** (戻り値の型が `I&`), **`i - n`** (戻り値の型が `I`) **が定義されている**
  ```cpp
  constexpr iterator& operator-=(difference_type n) //
    requires std::ranges::random_access_range<View> {
    return *this += -n;
  }
  friend constexpr iterator operator-(iterator x, difference_type n) //
    requires std::ranges::random_access_range<View> {
    x -= n;
    return x;
  }
  ```
- **イテレータ間の減算 `-` が定義されており、戻り値の型が `typename I::difference_type` である**
  ```cpp
  friend constexpr difference_type //
  operator-(const iterator& x, const iterator& y) requires
    std::ranges::random_access_range<View> {
    return x.current_ - y.current_;
  }
  ```
- **添字演算子 `i[n]` が定義されており、戻り値の型が間接参照演算子 `*i` と同じである**
  ```cpp
  constexpr std::pair<std::size_t, std::ranges::range_reference_t<View>>
  operator[](difference_type n) const //
    requires std::ranges::random_access_range<View> {
    return *(*this + n);
  }
  ```

補足ですが、`operator<`、`operator+=`、イテレータ間の減算 `-` **以外**はボイラープレートです。

また、`I` が `std::random_access_iterator` コンセプトを満たすためには不要ですが、慣例に倣い以下の変更を加えます。

- **`I` に三方比較演算子 `<=>` を追加する**
  ```cpp
  friend constexpr auto operator<=>(const iterator& x, const iterator& y) //
    requires std::ranges::random_access_range<View> and                   //
    std::three_way_comparable<std::ranges::iterator_t<View>> {
    return x.current_ <=> y.current_;
  }
  ```

[本節の差分](link?)

## `sized_sentinel_for`・`sized_range` に対応する

`sized_range` とは償却定数時間でサイズを取得できる range のことです。また `sized_sentinel_for<I, S>` は番兵イテレータに対するコンセプトであり、`S` がイテレータ `I` との間の距離が計算できる番兵イテレータであることを表します。

償却定数時間でサイズを取得できる range の例として `std::ranges::random_access_range` コンセプトを満たす range が直ちに挙げられます。しかし、償却定数時間でサイズを取得できるには必ずしも `std::ranges::random_access_range` コンセプトを満たす必要はありません。そのような例として、 サイズをキャッシュしているコンテナ (`std::unordered_map` など) が挙げられます。そのため `V` が `std::ranges::sized_range` コンセプトを満たすか否か、および `S` が `std::sized_sentinel_for<I>` コンセプトを満たすか否かは、そのイテレータ型 `I` のイテレータコンセプトとは独立に規定されています。

本節では元の view が `sized_range` (または `sized_sentinel_for`) を満たす場合に、 `enumerate_view` が `sized_range` (または `sized_sentinel_for`) を満たすよう変更を加えます。

`S` が `std::sized_sentinel_for<I>` コンセプトを満たすには、以下の条件が成立する必要があります。

- イテレータと番兵イテレータ間の減算 `i - s, s - i` が定義されており、戻り値の型が `typename I::difference_type` である
  ```cpp
  friend constexpr std::ranges::range_difference_t<View> //
  operator-(const iterator& x, const sentinel& y) requires
    std::sized_sentinel_for<std::ranges::sentinel_t<View>,
                            std::ranges::iterator_t<View>> {
    return x.base() - y.end_;
  }
  friend constexpr std::ranges::range_difference_t<View>
  operator-(const sentinel& x, const iterator& y) requires
    std::sized_sentinel_for<std::ranges::sentinel_t<View>,
                            std::ranges::iterator_t<View>> {
    return x.end_ - y.base();
  }
  ```

`V` が `std::ranges::sized_range` コンセプトを満たすには、以下の条件が成立する必要があります。

- `V` にメンバ関数 `size()` が定義されている
  ```cpp
  constexpr auto size() requires std::ranges::sized_range<View> {
    return std::ranges::size(base_);
  }
  ```

[本節の差分](link?)

<!-- TODO: ここからはオプショナルです、と書く? -->

## `iterator_category` を定義する

[`random_access_iterator` に対応する](link?) までで、元の view がイテレータコンセプトを満たす場合に `enumerate_view` が同じイテレータコンセプトを満たす方法を紹介しました。しかし、現在の `enumerate_view` は C++17 以前のイテレータ要件 (規格では _Cpp17InputIterator_ などと呼ばれています) を満たしません。本節では `iterator_category` を定義することで C++17 以前のイテレータ要件を満足させます。

元の view が C++17 以前のイテレータ要件を満たす場合に `V` が同じイテレータ要件を満たすには、`I` が下記の構造体 `deduce_iterator_category` を継承している必要があります。

```cpp
template <class View>
struct deduce_iterator_category {};

template <class View>
requires requires {
  typename std::iterator_traits<
    std::ranges::iterator_t<View>>::iterator_category;
}
struct deduce_iterator_category<View> {
  using iterator_category = typename std::iterator_traits<
    std::ranges::iterator_t<View>>::iterator_category;
};
```

通常はこれでよいのですが、`enumerate_view` は間接参照演算子 `*i` が左辺値参照を返さないため、_Cpp17InputIterator_ よりも強いイテレータ要件を満たしません。そのため `enumerate_view` の `deduce_iterator_category` は以下のように修正する必要があります。

```diff cpp
  template <class View>
  struct deduce_iterator_category {};

  template <class View>
  requires requires {
    typename std::iterator_traits<
      std::ranges::iterator_t<View>>::iterator_category;
  }
  struct deduce_iterator_category<View> {
-   using iterator_category = typename std::iterator_traits<
-     std::ranges::iterator_t<View>>::iterator_category;
+   using iterator_category = std::input_iterator_tag;
  };
```

[本節の差分](link?)

## 必要に応じて `iter_move` を定義する

ここでは非メンバ関数 `iter_move(i)` を手動で定義するが必要があるときについて説明します。

`std::ranges::iter_move` は以下のように定義されています。

- 実引数依存の名前探索によって `iter_move(std::forward<I>(i))` が見つかる場合、それを呼び出します
- そうでなくて `*std::forward<I>(i)` が well-formed な左辺値の場合、`std::move(*std::forward<I>(i))` を呼び出します
- そうでなくて `*std::forward<I>(i)` が well-formed な右辺値の場合、それを呼び出します
- そうでない場合、`std::ranges::iter_move(i)` は ill-formed です

すなわち `iter_move(i)` を手動で定義しなくても、`std::ranges::iter_move` は `std::move(*std::forward<I>(i))` か `*std::forward<I>(i)` を選択して適切に右辺値を返します。

しかし間接参照演算子 `*i` が左辺値を保持した右辺値(例えば `std::pair<T&, U&>` など)を返す場合に、上記のデフォルトの定義では上手くムーブすることができません。そのような場合に `iter_move(i)` を手動で定義します。

`enumerate_view` の戻り値は `std::pair<std::size_t, std::ranges::range_reference_t<View>>` であるため、上記の場合に該当します。このとき `enumerate_view` の `iter_move(i)` は以下のように定義できます。

```cpp
friend constexpr std::pair<std::size_t,
                           std::ranges::range_rvalue_reference_t<View>>
iter_move(const iterator& x) noexcept(
  noexcept(std::ranges::iter_move(x.current_))) {
  return {x.count_, std::ranges::iter_move(x.current_)};
}
```

[本節の差分](link?)

## `common_range` に対応する

`common_range` とは、イテレータと番兵イテレータの型が一致する range のことです。

```cpp
std::forward_list<int> fl{};
// fl は common_range
static_assert(std::ranges::common_range<decltype(fl)>);
std::ranges::take_view taken(fl, 0);
// taken は common_range ではない
static_assert(not std::ranges::common_range<decltype(taken)>);
```

C++17 以前のイテレータではイテレータと番兵イテレータの型が一致することは前提とされていました。しかし C++20 以降ではこの 2 者は必ずしも一致しないものとして扱われています。本節では元の view が `common_range` である場合に `enumerate_view` が `common_range` となるよう、以下の変更を加えます。

```diff cpp
- constexpr auto end() { return sentinel(std::ranges::end(base_)); }
+ constexpr auto end() {
+   if constexpr (std::ranges::common_range<View>)
+     return iterator(std::ranges::end(base_), std::ranges::size(base_));
+   else
+     return sentinel(std::ranges::end(base_));
+ }
```

通常はこれでよいのですが、`enumerate_view` の イテレータの構築には、元となる view のサイズを必要とします。そのため `enumerate_view` の場合は `std::ranges::common_range` に加えて `std::ranges::sized_range` で制約する必要があります。

```diff cpp
  constexpr auto end() {
-   if constexpr (std::ranges::common_range<View>)
+   if constexpr (std::ranges::common_range<View> and std::ranges::sized_range<View>)
      return iterator(std::ranges::end(base_), std::ranges::size(base_));
    else
      return sentinel(std::ranges::end(base_));
  }
```

[本節の差分](link?)

## _const-iterable_ に対応する

_const-iterable_ とは、const 修飾後も range コンセプトを満たす range のことです。

```cpp
const std::vector<int> v{};
// v は const-iterable
static_assert(std::ranges::range<decltype(v)>);
const std::ranges::filter_view filtered(v, std::identity{});
// filtered は const-iterable ではない
static_assert(not std::ranges::range<decltype(filtered)>);
```

STL のコンテナなど、一般的な range は _const-iterable_ ですが、現在の `enumerate_view` はその要件を満たしません。本節では元の view が _const-iterable_ である場合に、 `enumerate_view` が _const-iterable_ となるよう変更を加えます。

ここでの変更は多岐に渡るため、差分の多くは [リンク先](link?) に譲り、変更の概略を紹介します。

#### `enumerate_view<View>::iterator` および `enumerate_view<View>::sentinel` に対する変更

1. 非型テンプレートパラメータ `bool Const` を追加する
   ```diff cpp
   + template <bool Const>
     struct iterator;
   + template <bool Const>
     struct sentinel;
   ```
2. 型エイリアス `Base` を、`Const` が真のとき `const View`、偽のとき `View` として定義する
   ```diff cpp
   + using Base = std::conditional_t<Const, const View, View>;
   ```
3. すべての `View` を `Base` で置き換える

#### `enumerate_view` に対する変更

1. メンバ関数 `begin()` を `iterator<false>` を返すよう変更する
   ```diff cpp
   - constexpr iterator begin() { return {std::ranges::begin(base_), 0}; }
   + constexpr iterator<false> begin() { return {std::ranges::begin(base_), 0}; }
   ```
2. メンバ関数 `end()` を `sentinel<false>` (`common_range` の場合は `iterator<false>`) を返すよう変更する
   ```diff cpp
    constexpr auto end() {
      if constexpr (std::ranges::common_range<View> and //
                    std::ranges::sized_range<View>)
   -    return iterator(std::ranges::end(base_), std::ranges::size(base_));
   +    return iterator<false>(std::ranges::end(base_), std::ranges::size(base_));
      else
   -    return sentinel(std::ranges::end(base_));
   +    return sentinel<false>(std::ranges::end(base_));
    }
   ```
3. メンバ関数 `begin() const` を定義し、 `iterator<true>` を返すようにする
   ```diff cpp
   + constexpr iterator<true>
   + begin() const requires std::ranges::input_range<const View> {
   +   return {std::ranges::begin(base_), 0};
   + }
   ```
4. メンバ関数 `end() const` を定義し、 `sentinel<true>` (`common_range` の場合は `iterator<true>`) を返すようにする
   ```diff cpp
   + constexpr auto end() const requires std::ranges::input_range<const View> {
   +   if constexpr (std::ranges::common_range<const View> and //
   +                 std::ranges::sized_range<const View>)
   +     return iterator<true>(std::ranges::end(base_), std::ranges::size(base_));
   +   else
   +     return sentinel<true>(std::ranges::end(base_));
   + }
   ```

[本節の差分](link?)

## range adaptor object/range adaptor closure object を定義する (C++23 以降)

range adaptor の構築方法には、直接コンストラクタを呼び出す方法の他にも、専用のヘルパー関数オブジェクトを呼び出す方法があります。

```cpp
std::vector<int> v{0, 1, 2};
auto pred = [](auto x) { return x % 2 == 0; };
// 直接コンストラクタを呼び出すことで構築する
std::ranges::filter_view filtered(v, pred);
// 専用のヘルパー関数オブジェクトを用いて構築する
auto filtered2 = std::views::filter(v, pred);
```

また、下記のようにパイプライン記法を用いて構築することもできます。

```cpp
// パイプライン記法を用いて構築する
auto filtered3 = v | std::views::filter(pred);
```

本節では `enumerate_view` においてこのような記法ができるよう、ヘルパー関数オブジェクトを実装します。なお、本節のコードは C++23 に向けて採択されたライブラリ機能 ([P2387R3 Pipe support for user-defined range adaptors](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2387r3.html)) を用いているため、コンパイルできるのは C++23 に対応したコンパイラのみとなります。

`enumerate_view` のヘルパー関数オブジェクトは以下のように実装することができます。

```cpp
struct enumerate_fn : std::ranges::range_adaptor_closure<enumerate_fn> {
  template <std::ranges::viewable_range Range>
  constexpr auto operator()(Range&& range) const
    noexcept(noexcept(enumerate_view(std::forward<Range>(range)))) {
    return enumerate_view(std::forward<Range>(range));
  }
};

inline namespace cpo {
  inline constexpr auto enumerate = enumerate_fn();
} // namespace cpo
```

補足ですが、このようなヘルパー関数オブジェクトの実装方法は、対応する view のコンストラクタの引数によって異なります。`enumerate_view` のように元となる view のみを受け取る view のヘルパー関数オブジェクトは `enumerate_view` と同様に実装することができます。一方、`filter_view` のように元となる view の他にも引数を受け取る場合は、ヘルパー関数オブジェクトの実装方法は少し異なります。参考のため、`filter_view` のヘルパー関数オブジェクトの実装例を以下に示します。

```cpp
struct filter_fn {
  template <std::ranges::viewable_range Range, class Pred>
  constexpr auto operator()(Range&& range, Pred&& pred) const
    noexcept(noexcept(
      std::ranges::filter_view(std::forward<Range>(range), std::forward<Pred>(pred)))) {
    return
      std::ranges::filter_view(std::forward<Range>(range), std::forward<Pred>(pred));
  }
  template <class Pred>
  requires std::constructible_from<std::decay_t<Pred>, Pred>
  constexpr auto operator()(Pred&& pred) const
    noexcept(noexcept(std::is_nothrow_constructible_v<std::decay_t<Pred>, Pred>)) {
    return
      std::ranges::range_adaptor_closure(std::bind_back(*this, std::forward<Pred>(pred)));
  }
};

inline namespace cpo {
  inline constexpr auto filter = filter_fn();
} // namespace cpo
```

<!-- TODO: inline namespace について書く -->

本節ではヘルパー関数オブジェクトの詳細まで立ち入ることはできませんでした。その詳細については [［C++］ ranges のパイプにアダプトするには — 地面を見下ろす少年の足蹴にされる私](https://onihusube.hatenablog.com/entry/2022/04/24/010041) において詳しく説明されています。

[本節の差分](link?)

## 補足: `enumerate_view` の提案について

本記事で紹介した `enumerate_view` は、ライブラリ機能として追加されることが提案されています。

- [P2164R6 `views::enumerate`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2164r6.pdf)

本記事の `enumerate_view` は提案されているものとほとんど同じですが、簡単のため以下の点で差異があります。

- 提案の `enumerate_view` は index の型が `std::size_t` ではなく、以下で定義される `index_type` です
  <!-- TODO: index_type の定義 -->
- 提案の `enumerate_view` は index の型が const 修飾されています
- 提案の `enumerate_view` の値型は、`std::pair` ではなく専用の型 `std::enumerate_result` になります。この `std::enumerate_result` は構造化束縛に対応しています
