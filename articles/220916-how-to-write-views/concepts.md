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
- 前置インクリメント `++i` が定義されており、返り値の型が `I&` である
- 後置インクリメント `i++` が定義されている
- 間接参照演算子 `*i` が定義されており、返り値の型が参照修飾できる

### `S` を `sentinel_for` に対応させる

```cpp
template<class S, class I>
  concept sentinel_for =
    std::semiregular<S> &&
    std::input_or_output_iterator<I> &&
    __WeaklyEqualityComparableWith<S, I>;
```

- S が std::semiregular コンセプトを満たす (すなわちムーブ・コピー・デフォルト初期化可能である)
- 等値比較演算子 i == s が定義されている

### `V` を `view` に対応させる

- V が std::movable コンセプトを満たす
- メンバ関数 begin が定義されている
- メンバ関数 end が定義されている
- V が std::ranges::view_interface を継承している

## `input_iterator`

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

- I が std::input_or_output_iterator コンセプトを満たす
- I に typename I::value_type が定義されており、その型がオブジェクト型である
- std::iterator_traits<I>が特殊化されておらず、I に typename I::iterator_concept が定義されており、その型が std::input_iterator_tag を継承している
- [追加] 必要に応じてメンバ関数 iter_move が定義されている
<!-- lvalue-referenceを保持する型(std::pair<std::size_t, T&>など)を返すイテレータは、iter_moveを定義した方がよい -->

## `forward_iterator`

```cpp
template<class I>
  concept forward_iterator =
    std::input_iterator<I> &&
    std::derived_from</*ITER_CONCEPT*/<I>, std::forward_iterator_tag> &&
    std::incrementable<I> &&
    std::sentinel_for<I, I>;
```

- I が std::input_iterator コンセプトを満たす
- I が std::semiregular コンセプトを満たす(すなわち上記(ムーブ可能)に加え、コピー・デフォルト初期化可能である)
- typename I::iterator_concept が std::forward_iterator_tag を継承している
- 等値比較演算子 == が定義されている
- 後置インクリメント i++ の返り値の型が I である

## `bidirectional_iterator`

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

- I が std::forward_iterator コンセプトを満たす
- typename I::iterator_concept が std::bidirectional_iterator_tag を継承している
- 前置デクリメント --i が定義されており、返り値の型が I& である
- 後置デクリメント i-- が定義されており、返り値の型が I である

## `random_access_iterator`

※ n の型は iter_difference_t とする

- I が std::bidirectional_iterator コンセプトを満たす
- typename I::iterator_concept が std::random_access_iterator_tag を継承している
- 比較演算子 <, >, <=, => が定義されている
- イテレータと数値の加算 i += n (返り値の型が I&), i + n, n + i (返り値の型が I) が定義されている
- イテレータと数値の減算 i -= n (返り値の型が I&), i - n (返り値の型が I) が定義されている
- イテレータ間の減算 - が定義されており、返り値の型が typename I::difference_type である
- 添字演算子 i[n] が定義されており、返り値の型が間接参照演算子と同じである
