---
title: "`MDSPAN` (3): LayoutPolicy"
emoji: ""
type: "tech"
topics: ["cpp", "cpp23"]
published: false
---

## `LayoutPolicy` の概要
`LayoutPolicy` とは以下の制約を満たすクラスの総称です。

- クラステンプレート `template<class Extents> mapping` をもつ
  - その型は `Mapping` (後述) である

(例)
```cpp
struct LayoutPolicy {
  template<class Extents>
  class mapping;
};
```

(具体例)
- `layout_left`
- `layout_right`
- `layout_stride`

## `Mapping` の概要
`Mapping` とは以下の制約を満たすクラスの総称です。

- エイリアス `extents_type` をもつ
  - その型は `extents` の特殊化である
- エイリアス `layout_type` をもつ
  - その型は `LayoutPolicy` である
- メンバ関数 `extents()` をもつ
  - `Mapping` の保持する extents を返す
- メンバ関数 `operator()(SizeType... indices)` をもつ
  - 多次元インデックス `indices...` をメモリ上の1点を指す単一インデックスにマップした値を返す
- メンバ関数 `required_span_size()` をもつ
  - 参照しているシーケンスが空の場合0を、そうでない場合この `Mapping` の `operator()` が返す値の最大値を返す
- メンバ関数 `is_unique()` をもつ
  - この `Mapping` の `operator()` が単射の場合 true を、そうでない場合 false を返す
- メンバ関数 `is_contiguous()` をもつ
  - この `Mapping` の `operator()` が全射の場合 true を、そうでない場合 false を返す
- メンバ関数 `is_strided()` をもつ
  - この `Mapping` の `operator()(indices...)` の値と `operator()((indices + dr)...)` の値の差 `sr` が `indices...` に依らず `dr` にのみ依存するとき true を、そうでない場合 false を返す
- 静的メンバ関数 `is_always_unique()` をもつ
  - `is_unique` であることがコンパイル時にわかるとき true を返す
- 静的メンバ関数 `is_always_contiguous()` をもつ
  - `is_contiguous` であることがコンパイル時にわかるとき true を返す
- 静的メンバ関数 `is_always_strided()` をもつ
  - `is_strided` であることがコンパイル時にわかるとき true を返す
- メンバ関数 `stride(SizeType r)` をもつ
  - `is_strided()` が成り立つとき、`is_strided()` における `sr` を返す
  <!-- TODO: rが説明に出てこない -->

(例)
```cpp
  // NOTE: layout_left::mapping より拝借
  template<class Extents>
  class mapping {
  public:
    using size_type = typename Extents::size_type;
    using extents_type = Extents;
    using layout_type = layout_left;

    constexpr mapping() noexcept = default;
    constexpr mapping(const mapping&) noexcept = default;
    constexpr mapping(mapping&&) noexcept = default;
    constexpr mapping(const Extents&) noexcept;
    template<class OtherExtents>
      explicit(!std::is_convertible_v<OtherExtents,Extents>)
      constexpr mapping(const mapping<OtherExtents>&) noexcept;
    template<class OtherExtents>
      explicit(!std::is_convertible_v<OtherExtents,Extents>)
      constexpr mapping(const layout_right::template mapping<OtherExtents>&) noexcept
      requires(rank() <= 1);
    template<class OtherExtents>
      explicit(rank() > 0) constexpr mapping(
      const layout_stride::template mapping<OtherExtents>& other);

    constexpr mapping& operator=(const mapping&) noexcept = default;
    constexpr mapping& operator=(mapping&&) noexcept = default;

    constexpr Extents extents() const noexcept { return extents_; }

    constexpr size_type required_span_size() const noexcept;

    template<class... Indices>
      constexpr size_type operator()(Indices...) const noexcept;

    static constexpr bool is_always_unique() noexcept { return true; }
    static constexpr bool is_always_contiguous() noexcept { return true; }
    static constexpr bool is_always_strided() noexcept { return true; }

    constexpr bool is_unique() const noexcept { return true; }
    constexpr bool is_contiguous() const noexcept { return true; }
    constexpr bool is_strided() const noexcept { return true; }

    constexpr size_type stride(size_t) const noexcept;

    template<class OtherExtents>
      friend constexpr bool operator==(const mapping&, const mapping<OtherExtents>&) noexcept;

  private:
    Extents extents_{}; // exposition only
  };
```

(具体例)
- `layout_left::template mapping<Extents>`
- `layout_right::template mapping<Extents>`
- `layout_stride::template mapping<Extents>`

## `layout_left`
```cpp
struct layout_left {
  // i0 + (i1 + E(1) * (i2 + E(2) * i3))
  template <class Extents>
  class mapping {
    template <size_t r, size_t Rank>
    struct rank_count {};

    template <size_t r, size_t Rank, class I, class... Indices>
    constexpr size_t
    compute_offset(rank_count<r, Rank>, const I& i, Indices... indices) const {
      return i + compute_offset(rank_count<r + 1, Rank>(), indices...) * extents.template extent<r>();
    }

    template <class I>
    constexpr size_t
    compute_offset(rank_count<Extents::rank() - 1, Extents::rank()>, const I& i) const {
      return i;
    }

    constexpr size_t
    compute_offset(rank_count<0, 0>) const {
      return 0;
    }

  public:
    template <class... Indices>
    constexpr size_type
    operator()(Indices... indices) const noexcept {
      return compute_offset(rank_count<0, Extents::rank()>(), indices...);
    }
  };
};
```

## `layout_right`
```cpp
struct layout_left {
  // ((i0*E(1) + i1) * E(2) + i2) + i3
  template <class Extents>
  class mapping {
    template <size_t r, size_t Rank>
    struct rank_count {};

    template <size_t r, size_t Rank, class I, class... Indices>
    constexpr size_t
    compute_offset(size_t offset, rank_count<r, Rank>, const I& i, Indices... indices) const {
      return compute_offset(offset * extents.template extent<r>() + i, rank_count<r + 1, Rank>(),
                            indices...);
    }

    template <class I, class... Indices>
    constexpr size_t
    compute_offset(rank_count<0, Extents::rank()>, const I& i, Indices... indices) const {
      return compute_offset(static_cast<size_t>(i), rank_count<1, Extents::rank()>(), indices...);
    }

    constexpr size_t
    compute_offset(size_t offset, rank_count<Extents::rank(), Extents::rank()>) const {
      return offset;
    }

    constexpr size_t
    compute_offset(rank_count<0, 0>) const {
      return 0;
    }

  public:
    template <class... Indices>
    constexpr size_type
    operator()(Indices... indices) const noexcept {
      return compute_offset(rank_count<0, Extents::rank()>(), indices...);
    }
  };
};
```
