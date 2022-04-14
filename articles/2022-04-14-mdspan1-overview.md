---
title: "`MDSPAN`: Overview"
emoji: ""
type: "tech"
topics: ["cpp", "cpp23"]
published: false
---

## `span` vs `mdspan`
`mdspan` は 多次元 subscript access に対応した `span` です。
`m[i, j, k]` のように使える、メモリ連続性をもつシーケンスを参照するクラスと言えます。
しかし `span` とは異なり `mdspan` は以下の点で拡張されています。

- 要素数 `Extents` は `size_t` だけではなく複数の `size_t` を持ち得ます。
- 複数のインデックスからシーケンスの1点を指すインデックスへの変換をカスタマイズすることができます。これは `LayoutPolicy` (およびその内部クラスである `LayoutPolicy::template mapping<Extents>`) によって制御されます。
- シーケンスの1点を指すインデックスから要素への変換をカスタマイズすることができます。これは `AccessorPolicy` によって制御されます。

```cpp
  // [views.span], class template span
  template<class ElementType, size_t Extent = dynamic_extent>
    class span;

  // [mdspan.mdspan], class template mdspan
  template<class ElementType, class Extents, class LayoutPolicy = layout_right,
           class AccessorPolicy = default_accessor<ElementType>>
    class mdspan;
```

*`span` と `mdspan` の宣言。Reference: [[span.syn]](https://timsong-cpp.github.io/cppwp/n4861/span.syn) and [[mdspan.syn]](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0009r14.html#:~:text=Header%20%3Cmdspan%3E%20synopsis%20%5Bmdspan.syn%5D)*
