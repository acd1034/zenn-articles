## `view` コンセプトに対応する

- `V` が[後述の条件](link?)を満たす
- `I` が `std::input_or_output_iterator` コンセプトを満たす
- `S` が `std::sentinel_for<I>` コンセプトを満たす

### `I` を `input_or_output_iterator` に対応させる

```cpp
template<class I>
  concept weakly_incrementable =
    std::movable<I> &&
    requires(I i) {
      typename std::iter_difference_t<I>;
      requires /*is-signed-integer-like*/<std::iter_difference_t<I>>;
      { ++i } -> std::same_as<I&>;
      i++;
    };

template <class I>
concept input_or_output_iterator =
  requires(I i) {
    { *i } -> /*can-reference*/;
  } &&
  std::weakly_incrementable<I>;
```

- `I` が `std::movable` コンセプトを満たす
- `I` に `typename I::difference_type` が定義されており、その型が符号付き整数型である
- 前置インクリメント `++i` が定義されており、戻り値の型が `I&` である
- 後置インクリメント `i++` が定義されている
- 間接参照演算子 `*i` が定義されており、戻り値の型が参照修飾できる

### `S` を `sentinel_for` に対応させる

```cpp
template<class S, class I>
  concept sentinel_for =
    std::semiregular<S> &&
    std::input_or_output_iterator<I> &&
    __WeaklyEqualityComparableWith<S, I>;
```

- `S` が `std::semiregular` コンセプトを満たす (すなわちムーブ・コピー・デフォルト初期化可能である)
- 等値比較演算子 `i == s` が定義されている

### `V` を `view` に対応させる

- `V` が `std::movable` コンセプトを満たす
- メンバ関数 `begin()` が定義されている
- メンバ関数 `end()` が定義されている
- `V` が `std::ranges::view_interface` を継承している

## `input_iterator` に対応する

```cpp
template< class In >
  concept __IndirectlyReadableImpl =
    requires(const In in) {
      typename std::iter_value_t<In>;
      typename std::iter_reference_t<In>;
      typename std::iter_rvalue_reference_t<In>;
      { *in } -> std::same_as<std::iter_reference_t<In>>;
      { ranges::iter_move(in) } -> std::same_as<std::iter_rvalue_reference_t<In>>;
    } &&
    std::common_reference_with<
      std::iter_reference_t<In>&&, std::iter_value_t<In>&
    > &&
    std::common_reference_with<
      std::iter_reference_t<In>&&, std::iter_rvalue_reference_t<In>&&
    > &&
    std::common_reference_with<
      std::iter_rvalue_reference_t<In>&&, const std::iter_value_t<In>&
    >;
template< class In >
  concept indirectly_readable =
    __IndirectlyReadableImpl<std::remove_cvref_t<In>>;

template<class I>
  concept input_iterator =
    std::input_or_output_iterator<I> &&
    std::indirectly_readable<I> &&
    requires { typename /*ITER_CONCEPT*/<I>; } &&
    std::derived_from</*ITER_CONCEPT*/<I>, std::input_iterator_tag>;
```

- `I` が `std::input_or_output_iterator` コンセプトを満たす
- `I` に `typename I::value_type` が定義されており、その型がオブジェクト型である
- ~~std::iterator_traits<I>が特殊化されておらず、~~
  `I` に `typename I::iterator_concept` が定義されており、その型が `std::input_iterator_tag` を継承している
- 必要に応じて非メンバ関数 `iter_move(i)` が定義されている
  <!-- lvalue-referenceを保持する型(std::pair<std::size_t, T&>など)を返すイテレータは、iter_moveを定義した方がよい -->

## `forward_iterator` に対応する

```cpp
template<class I>
  concept forward_iterator =
    std::input_iterator<I> &&
    std::derived_from</*ITER_CONCEPT*/<I>, std::forward_iterator_tag> &&
    std::incrementable<I> &&
    std::sentinel_for<I, I>;
```

- `I` が `std::input_iterator` コンセプトを満たす
- `I` が `std::semiregular` コンセプトを満たす(すなわち上記(ムーブ可能)に加え、コピー・デフォルト初期化可能である)
- `typename I::iterator_concept` が `std::forward_iterator_tag` を継承している
- 等値比較演算子 `==` が定義されている
- 後置インクリメント `i++` の戻り値の型が `I` である

## `bidirectional_iterator` に対応する

```cpp
template<class I>
  concept bidirectional_iterator =
    std::forward_iterator<I> &&
    std::derived_from</*ITER_CONCEPT*/<I>, std::bidirectional_iterator_tag> &&
    requires(I i) {
      { --i } -> std::same_as<I&>;
      { i-- } -> std::same_as<I>;
    };
```

- `I` が `std::forward_iterator` コンセプトを満たす
- `typename I::iterator_concept` が `std::bidirectional_iterator_tag` を継承している
- 前置デクリメント `--i` が定義されており、戻り値の型が `I&` である
- 後置デクリメント `i--` が定義されており、戻り値の型が `I` である

## `random_access_iterator` に対応する

※ n の型は iter_difference_t とする

- `I` が `std::bidirectional_iterator` コンセプトを満たす
- `typename I::iterator_concept` が `std::random_access_iterator_tag` を継承している
- 比較演算子 `<, >, <=, =>` が定義されている
- イテレータと数値の加算 `i += n` (戻り値の型が `I&`), `i + n, n + i` (戻り値の型が `I`) が定義されている
- イテレータと数値の減算 `i -= n` (戻り値の型が `I&`), `i - n` (戻り値の型が `I`) が定義されている
- イテレータ間の減算 `-` が定義されており、戻り値の型が `typename I::difference_type` である
- 添字演算子 `i[n]` が定義されており、戻り値の型が間接参照演算子 `*i` と同じである

## `sized_sentinel_for`・`sized_range` に対応する

sized_sentinel_for

- イテレータと番兵イテレータ間の減算 `i - s, s - i` が定義されており、戻り値の型が `typename I::difference_type` である

sized_range

- `V` にメンバ関数 `size()` が定義されている

## _simple-view_ に対応する

_simple-view_ とは、ある種の range を表す説明専用(標準ライブラリの内部実装で用いられており、表には現れない)コンセプトです。const 修飾後も range であり、かつ const 修飾前のイテレータ・番兵イテレータの型と、修飾後のイテレータ・番兵イテレータの型が一致するような range を表します。

## 参考文献

#### C++20 策定以後に採択された欠陥報告

- [P2325R3 Views should not be required to be default constructible](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2325r3.html)
  `view` コンセプト (厳密には `weakly_incrementable` コンセプト) に `default_initializable` コンセプトが要求されなくなりました。
- [P2415R2 What is a `view`?](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2415r2.html)
  `view` コンセプト に $O(1)$ コピー可能であることが要求されなくなりました。

#### C++23 に向けて採択された関連論文

- [P2494R2 Relaxing range adaptors to allow for move only types](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2494r2.html)
  view 内部で保持する型としてコピー構築不能だがムーブ構築可能な型が許可されるようになりました。

#### 処理系の対応状況

- [Implementation Status C++ 2023 — The GNU C++ Library Manual](https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.2023)
- [libc++ C++20 Status](https://libcxx.llvm.org/Status/Cxx20.html)
- [VS 2019 16.11.14 — VS 2019 Changelog](https://github.com/microsoft/STL/wiki/VS-2019-Changelog#vs-2019-161114)
- [VS 2022 17.1 — VS 2022 Changelog](https://github.com/microsoft/STL/wiki/Changelog#vs-2022-171)
- [VS 2022 17.0 — VS 2022 Changelog](https://github.com/microsoft/STL/wiki/Changelog#vs-2022-170)
- [MSVC’s STL Completes /std:c++20 — Microsoft C++ Team Blog](https://devblogs.microsoft.com/cppblog/msvcs-stl-completes-stdc20/)

#### `view` の書き方

- [24 Ranges library [ranges] — N4861](https://timsong-cpp.github.io/cppwp/n4861/ranges)
- [Tutorial: Writing your first view from scratch (C++20 / P0789) — Hannes Hauswedell's homepage](https://hannes.hauswedell.net/post/2018/04/11/view1/)
- [[C++]<ranges>の std::views と同様に扱える view を自作する。— 賢朽脳瘏](https://kenkyu-note.hatenablog.com/entry/2021/02/20/150512)
- [[C++]std::ranges::views::zip、enumerate の代替機能を作ってみる。— 賢朽脳瘏](https://kenkyu-note.hatenablog.com/entry/2021/02/21/035242)

#### `input_or_output_iterator` ~ `random_access_iterator`

- [23 Iterators library [iterators] — N4861](https://timsong-cpp.github.io/cppwp/n4861/iterators)
- [std::input_or_output_iterator — cpprefjp](https://cpprefjp.github.io/reference/iterator/input_or_output_iterator.html) ~ [std::random_access_iterator — cpprefjp](https://cpprefjp.github.io/reference/iterator/random_access_iterator.html)
  各種イテレータの最小実装例 (`sample_input_or_output_iterator` など) が掲載されています
- [［C++］ C++17 イテレータ <=> C++20 イテレータ != 0 — 地面を見下ろす少年の足蹴にされる私](https://onihusube.hatenablog.com/entry/2020/12/27/150400)
  C++20 以降のイテレータコンセプトについて、C++17 以前と比較して説明されています。
- [イテレータの解説をするなんて今更佳代 — qiita.com/yumetodo](https://qiita.com/yumetodo/items/245e94a0e85db9bf5cbb)
  C++17 イテレータの書き方について説明されています

#### `iterator_category`

- [23.3.5 C++17 iterator requirements [iterator.cpp17] — N4861](https://timsong-cpp.github.io/cppwp/n4861/iterator.cpp17)
  C++17 イテレータ要件について
- [23.3.2.3 Iterator traits [iterator.traits] — N4861](https://timsong-cpp.github.io/cppwp/n4861/iterator.traits)
  C++17 イテレータ要件の説明専用コンセプト
- [［C++］C++20 からの iterator_traits 事情 — 地面を見下ろす少年の足蹴にされる私](https://onihusube.hatenablog.com/entry/2020/12/14/002822)
  C++20 イテレータを C++17 イテレータとして扱う場合の `iterator_traits` の役割について説明されています

#### _const-iterable_

- [Range と View と const 修飾 — yohhoy の日記](https://yohhoy.hatenadiary.jp/entry/20210701/p1)
  range の const 修飾と view の const 修飾は異なることについて説明されています
- [filter_view が const-iterable ではない理由 — zenn.dev/onihusube/scraps](https://zenn.dev/onihusube/scraps/40a95c8f769414)
- [C++20 Range Adaptors and Range Factories — Barry's C++ Blog](https://brevzin.github.io/c++/2021/02/28/ranges-reference/)

<!-- emdash: `—` -->

#### range adaptor object/range adaptor closure object

- [P2387R3 Pipe support for user-defined range adaptors](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2387r3.html)
- [［C++］ ranges のパイプにアダプトするには — 地面を見下ろす少年の足蹴にされる私](https://onihusube.hatenablog.com/entry/2022/04/24/010041)

#### `enumerate_view`

- [P2164R6 `views::enumerate`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2164r6.pdf)
