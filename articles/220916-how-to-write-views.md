---
title: view の書き方
emoji: 🪟
type: tech
topics: [cpp, cpp20, cpp23]
published: false
---

## 導入

目標:

- view の書き方を step by step で説明する
  <!-- TODO: view→range adaptorに変更 -->

ここでは例として`enumerate_view`を実装します。
使用例:

```cpp
std::vector<char> v{'a', 'b', 'c'};
for (auto&& [index, value] : enumerate_view(v))
  std::cout << index << ':' << value << ','; // output: 0:a,1:b,2:c,
```

出発点:

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

:::message
節の最後にその節の変更点の差分をリンクで示しています。
:::

:::message
本記事は C++20 策定後の欠陥報告を反映しているため、C++20 をサポートしていても古いコンパイラでは動作しない可能性があります。本記事のコードは以下のコンパイラで動作することを確認しています。

- gcc 12.1.0
- clang 16.0.0
- Visual Studio 2019 version 16.11

また、以下のコンパイラで動作すると思われます。

- gcc 12.1.0 以上
- clang 16.0.0 以上
- Visual Studio 2019 version 16.11.14 以上
- Visual Studio 2022 version 17.2 以上

:::

## `view` コンセプトに対応する

実装する view の型を`V`、`V`のイテレータの型を`I`、`V`の番兵イテレータの型を`S`とします。この時`V`が`view` コンセプトを満たすには、以下の条件が成立する必要があります。

- `V` が[後述の条件](link?)を満たす
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
  sentinel() requires
    std::default_initializable<std::ranges::sentinel_t<View>> = default;
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

[`sized_sentinel_for`・`sized_range` に対応する](link?)まで、`enumerate_view<View>::sentinel` は触りません。

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

[`sized_sentinel_for`・`sized_range` に対応する](link?)まで、`enumerate_view` は触りません。

[本節の差分](link?)

## `input_iterator` に対応する

[random_access_range に対応する](link?)まで、`enumerate_view<View>::iterator` のみに変更を加えます。
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

なお、後置インクリメントはほとんどの場合に上記のコードで定義できます(そのようなコードをボイラープレートと呼びます)。
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

なお、後置デクリメントはボイラープレートです。
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

なお、`operator<`, `operator+=`, イテレータ間の減算 `-` **以外**はボイラープレートです。
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

なお、`S` が `std::sized_sentinel_for<I>` コンセプトを満たすか否か、`V` が `std::ranges::sized_range` コンセプトを満たすか否かは、`I` の イテレータコンセプトとは独立に規定されています。そのため、`I` が `std::input_iterator` コンセプトから `std::random_access_iterator` コンセプトまでのどのコンセプトを満たすかに依らず、上記のメソッドは定義できます。

[本節の差分](link?)

