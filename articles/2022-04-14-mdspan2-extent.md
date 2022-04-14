---
title: "`MDSPAN` (2): `extents`"
emoji: ""
type: "tech"
topics: ["cpp", "cpp23"]
published: false
---

## `extents` の概要
```cpp
template<size_t... Extents>
class extents {
public:
  using size_type = size_t;

  // [mdspan.extents.cons], Constructors and assignment
  constexpr extents() noexcept = default;
  constexpr extents(const extents&) noexcept = default;
  constexpr extents& operator=(const extents&) noexcept = default;
  constexpr extents(extents&&) noexcept = default;
  constexpr extents& operator=(extents&&) noexcept = default;

  template<size_t... OtherExtents>
    explicit(see below)
    constexpr extents(const extents<OtherExtents...>&) noexcept;
  template<class... SizeTypes>
    explicit constexpr extents(SizeTypes...) noexcept;
  template<class SizeType, size_t N>
    explicit(N != rank_dynamic())
    constexpr extents(const array<SizeType, N>&) noexcept;

  // [mdspan.extents.obs], Observers of the domain multidimensional index space
  static constexpr size_t rank() noexcept { return sizeof...(Extents); }
  static constexpr size_t rank_dynamic() noexcept
    { return ((Extents == dynamic_extent) + ...); }
  static constexpr size_type static_extent(size_t) noexcept;
  constexpr size_type extent(size_t) const noexcept;

  // [mdspan.extents.compare], extents comparison operators
  template<size_t... OtherExtents>
    friend constexpr bool operator==(const extents&, const extents<OtherExtents...>&) noexcept;

private:
  static constexpr size_t dynamic_index(size_t) noexcept; // exposition only
  static constexpr size_t dynamic_index_inv(size_t i) noexcept; // exposition only
  array<size_type, rank_dynamic()> dynamic_extents_{}; // exposition only
};
```

`extents` は `sizeof...(Extents)` 個の extent を保持するクラスです。
静的な extent はテンプレートパラメータパック `Extents...` に保持され、動的な extent は `dynamic_extents_` に保持されます。
r番目の extent は　`e.extent(r)` とすることで確認できます。

## コンストラクタ
- 複数の extent から初期化するコンストラクタ
  ```cpp
  template<class... SizeTypes>
  explicit constexpr extents(SizeTypes... exts) noexcept;
  ```
  1. `sizeof...(SizeTypes)` が `dynamic_rank()` に一致する場合、`dynamic_extents_` を `exts...` で初期化します。
  2. `sizeof...(SizeTypes)` が `rank()` に一致する場合、`Extents...` が `dynamic_extent` (Ref. [span.syn]) に一致する `exts...` のみ取り出して `dynamic_extents_` を初期化します。
  ```cpp
  // 例
  using extents = stdex::extents<3, stdex::dynamic_extent, 7>;
  // 1.
  constexpr std::size_t ignoreme = 42;
  constexpr extents e1{ignoreme, 10, ignoreme};
  for (std::size_t i = 0; i < e1.rank(); ++i)
    std::cout << e1.extent(i) << std::endl; // 3 10 7
  // 2.
  constexpr extents e2{10};
  for (std::size_t i = 0; i < e1.rank(); ++i)
    std::cout << e2.extent(i) << std::endl; // 3 10 7
  ```

## オブザーバー
- `rank`: `Extents...` の個数
  ```cpp
  static constexpr size_t rank() noexcept { return sizeof...(Extents); }
  ```

- `rank_dynamic`: `Extents...` のうち `dynamic_extent` に一致するものの個数
  ```cpp
  static constexpr size_t rank_dynamic() noexcept
    { return ((Extents == dynamic_extent) + ...); }
  ```

- `static_extent`: r 番目の `Extents...`
  ```cpp
  constexpr size_type static_extent(size_t r) const noexcept;
  ```

- `extent`: r 番目の動的な extent
  ```cpp
  constexpr size_type extent(size_t r) const noexcept;
  ```
  1. `static_extent(r) == dynamic_extent` が成り立つとき、`dynamic_extents_[dynamic_index(r)]` を返します。
  2. そうでない場合、`static_extent(r)` を返します。
