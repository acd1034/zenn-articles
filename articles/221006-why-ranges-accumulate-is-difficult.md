---
title: なぜ ranges::accumulate は難しいのか
emoji: 📚
type: tech
topics: [cpp, cpp23, ranges]
published: false
---

<!-- TODO: 概要を書く -->

## はじめに

C++20 で範囲ライブラリが導入されたことで、リスト操作が容易に行えるようになりました。個人的に 3 大リスト操作を挙げるとすると、filter、map (あるいは transform)、fold (あるいは accumulate) ではないかと思います。しかし `ranges::accumulate` は C++20 入りを果たしませんでした。

その理由は、単に時間が足りなかったからだとされています[^why]。しかし、それは議論すべき点がなかったという意味ではないようです。実際、`ranges::accumulate` の代替である `ranges::fold` が C++23 に向けて採択されるまでに、以下の変遷を経ました。

- range-v3 で accumulate が実装される ([このコミット](https://github.com/ericniebler/range-v3/commit/8e1302be07b58da25b81383f4df4532df21960a1) が最初期の accumulate であるようです)
- [P1813 A Concept Design for the Numeric Algorithms](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1813r0.pdf) が提案される
- [P2214 A Plan for C++23 Ranges](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2214r0.html) にて accumulate は不適切であることが示唆される
- [P2322 `ranges::fold`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2322r6.html) が提案、のちに採択される
<!-- NOTE: Range TS に当初 accumulate は入っていたと思われるので、このbrief historyは誤りかもしれない -->

[^why]: [Why didn't `accumulate` make it into Ranges for C++20? - Stack Overflow](https://stackoverflow.com/questions/63933163/why-didnt-accumulate-make-it-into-ranges-for-c20)

accumulate は範囲ライブラリの実装経験である range-v3 で実装されており、その型制約は現在以下のようになっています[^range-v3]。この型制約は [最初期](https://github.com/ericniebler/range-v3/commit/8e1302be07b58da25b81383f4df4532df21960a1) よりコンセプト設計の変更を受けて何度も修正されているものの、骨子は変化していないようです。

```cpp
template <class F, class I1, class I2>
concept indirectly_binary_invocable =
  std::indirectly_readable<I1> and
  std::indirectly_readable<I2> and
  std::copy_constructible<F> and
  std::invocable<F&, std::iter_value_t<I1>&, std::iter_value_t<I2>&> and
  std::invocable<F&, std::iter_value_t<I1>&, std::iter_reference_t<I2>> and
  std::invocable<F&, std::iter_reference_t<I1>, std::iter_value_t<I2>&> and
  std::invocable<F&, std::iter_reference_t<I1>, std::iter_reference_t<I2>> and
  std::invocable<F&, std::iter_common_reference_t<I1>, std::iter_common_reference_t<I2>> and
  __common_reference_with<
    std::invoke_result_t<F&, std::iter_value_t<I1>&, std::iter_value_t<I2>&>,
    std::invoke_result_t<F&, std::iter_value_t<I1>&, std::iter_reference_t<I2>>,
    std::invoke_result_t<F&, std::iter_reference_t<I1>, std::iter_value_t<I2>&>,
    std::invoke_result_t<F&, std::iter_reference_t<I1>, std::iter_reference_t<I2>>>;

template <std::input_iterator I, std::sentinel_for<I> S, class T,
          class Op = std::plus<>, class Proj = std::identity>
requires indirectly_binary_invocable<Op&, T*, std::projected<I, Proj>> and
  std::assignable_from<T&, std::indirect_result_t<Op&, T*, std::projected<I, Proj>>>
constexpr T accumulate(I first, S last, T init, Op op = {}, Proj proj = {});
```

この型制約でも上手くいくように見えますが、この accumulate は採択されませんでした。

[^range-v3]: [range-v3/accumulate.hpp at master · ericniebler/range-v3](https://github.com/ericniebler/range-v3/blob/689b4f3da769fb21dd7acf62550a038242d832e5/include/range/v3/numeric/accumulate.hpp#L36-L42) 可能な限り C++20 の言葉に書き換えてあります

それではなぜ `ranges::accumulate` は　 range-v3 と同様の型制約で採択されなかったのでしょうか。また一方で、`ranges::fold` はなぜ採択されたのでしょうか。

これらの問いは STL におけるコンセプトの設計と密接な関係がありそうです。そこで本稿ではまず、 STL におけるコンセプトの設計思想について説明します。続いてそれに基づいて、`ranges::accumulate` が採択されなかった理由、および `ranges::fold` が採択された理由の説明を試みます。

本稿は規格関連文書の内容に加え、筆者の考察も含まれます。筆者の不勉強のために、事実とは異なる記述がある可能性があります。お気づきの点がございましたら、コメント頂けますと幸いです。

## STL におけるコンセプトの設計

<!-- NOTE: 要注意用語: 要件(requirement)、制約(constraint)、コンセプト -->

STL においてコンセプトは、一般的なアルゴリズムを記述するための抽象的な型に対する要件と位置付けられています[^sle2011]。一般性のあるコンセプトの設計指針は、コンセプトの粒度の観点で 2 種類存在します。

[^sle2011]: [Design of Concept Libraries for C++](https://www.stroustrup.com/sle2011-concepts.pdf)

1. コンセプトをできる限り大きくする。すなわち要件の多いコンセプトをごく少数のみ用意する

   - **例**: イテレータは random access iterator のみ
   - **利点**: コンセプトの意味が単純となる。ユーザが使用しやすくなる
   - **欠点**: 要件が過剰となる。アルゴリズムの実装において決して使用されない操作による要件が入る。その結果、有意義なクラスが要件を満たさない場面が生じる

2. コンセプトを可能な限り小さくする。すなわち要件の少ないコンセプトを多数用意する

   - **例**: `HasPlus`, `HasMinus`, `HasComma`, ...
     (C++0x に向けて考案され、削除されたコンセプトはこの形でした[^n2914])
   - **利点**: アルゴリズムの要件が限界まで緩和される。その結果、可能な限り多くのクラスがアルゴリズムを利用できるようになる
   - **欠点**: 要件の意味が不明確となる。アルゴリズムの中心的な要件が伝わらなくなる

[^n2914]: [N2914](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2009/n2914.pdf)

STL は両者のバランスをとることを目指しています。すなわち複数のクラスに適用可能な普遍的なアルゴリズムを提供しつつも、各コンセプトから意味が失われないコンセプト設計を目指しています。<!-- TODO: 文構造の反復を解消する -->

### コンセプトの指針

上述の目的を達成するために、STL はコンセプトに強い指針を与えています。それは一言で言うと<!-- TODO: 「一言」よりも適切な言葉を探す -->

$$
Concepts = Constraints + Axioms
$$

と表されます。

ここで _Constraints_ とは、コンパイル時に検査することのできる構文要件 (syntatic requirements) を表します。一方 _Axioms_ とは、実行時に検査することのできる意味要件 (semantic requirements) を意味します。STL は、コンセプトは構文要件と意味要件の両方を有する要件であると設計しています。<!-- TODO: 「設計」よりも適切な言葉を探す -->

STL はコンセプトを意味のある一般的な要件にするために、とりわけ以下の 2 点に注意して設計を行なっています。<!-- TODO: 文章が何か変 -->

1. コンセプトに必要な操作をすべて含める[^ccg-t21]
2. コンセプトに有意義な意味論をもたせる[^ccg-t20]

[^ccg-t21]: [T.21: Require a complete set of operations for a concept](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#t21-require-a-complete-set-of-operations-for-a-concept) 翻訳は [C++ Core Guidelines タイトル日本語訳 - Qiita/tetsurom](https://qiita.com/tetsurom/items/322c7b58cddaada861ff#tconceptsdef-concept-definition-rules--%E3%82%B3%E3%83%B3%E3%82%BB%E3%83%97%E3%83%88%E5%AE%9A%E7%BE%A9%E6%99%82%E3%81%AE%E3%83%AB%E3%83%BC%E3%83%AB) を参考にしました
[^ccg-t20]: [T.20: Avoid “concepts” without meaningful semantics](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#t20-avoid-concepts-without-meaningful-semantics) 翻訳は [C++ Core Guidelines タイトル日本語訳 - Qiita/tetsurom](https://qiita.com/tetsurom/items/322c7b58cddaada861ff#tconceptsdef-concept-definition-rules--%E3%82%B3%E3%83%B3%E3%82%BB%E3%83%97%E3%83%88%E5%AE%9A%E7%BE%A9%E6%99%82%E3%81%AE%E3%83%AB%E3%83%BC%E3%83%AB) を参考にしました

#### ダメな例

```cpp
template <class T>
concept Subtractable = requires(T a, T b) {
  a - b;
};
```

- **理由**: 制約が小さすぎるため、有意義な意味論を持たせることができない。その結果、_Subtractable_ コンセプトのみ満たすクラスにおいて十分一般的なアルゴリズムを提供することができない。
- **修正案** (一例):
  `a`, `b` を `T` 型のオブジェクトとして、
  - 次の構文要件を追加する: `+a`, `-a`, `a += b`, `a -= b`, `a + b`
  - 次の意味要件を追加する: `+a == a`, `(a -= b) == a += (-b)`, `a + b == (a += b)`, `a - b == a + (-b)`
  ```cpp
  template <class T>
  concept Subtractable = requires(T a, T b) {
    +a;
    -a;
    a += b;
    a -= b;
    a + b;
    a - b;
  } /* and axioms(T a, T b) {
    +a == a;
    (a -= b) == (a += (-b));
    a + b == (a += b);
    a - b == a + (-b);
  } */;
  ```

#### STL のコンセプトの例 1: random access iterator[^raiter]

```cpp
template <class I>
  concept random_access_iterator =
    bidirectional_iterator<I> and
    derived_from<ITER_CONCEPT(I), random_access_iterator_tag> and
    totally_ordered<I> and
    sized_sentinel_for<I, I> and
    requires(I i, const I j, const iter_difference_t<I> n) {
      { i += n } -> same_as<I&>;
      { j +  n } -> same_as<I>;
      { n +  j } -> same_as<I>;
      { i -= n } -> same_as<I&>;
      { j -  n } -> same_as<I>;
      {  j[n]  } -> same_as<iter_reference_t<I>>;
    };
```

- **意味要件** (一部):
  `i`, `j` を `I` 型のオブジェクト、 `n` を `iter_difference_t<I>` 型のオブジェクトとして、
  ```cpp
  i + n == (i += n);
  n + i == i + n;
  (i -= n) == (i += (-n));
  i - n == i + (-n);
  i[n] == *(i + n);
  if j - i == n, i + n == j;
  ```

[^raiter]: [[iterator.concept.random.access]](http://eel.is/c++draft/iterator.concept.random.access)

#### STL のコンセプトの例 2: view[^view]

```cpp
template <class T>
  concept view =
    range<T> and movable<T> and enable_view<T>;
```

- **意味要件**:
  - `T` は $O(1)$ でムーブ構築可能
  - `T` のムーブ代入の計算量は破棄とムーブ構築の複合操作と同等かそれ以下
  - $M$ 要素保持する `T` を $N$ 回コピーまたはムーブしたオブジェクトは、$O(N + M)$ で破棄可能
  - `T` はコピー構築不能か、$O(1)$ でコピー構築可能
  - `T` はコピー不能か、コピー代入の計算量が破棄とコピー構築の複合操作と同等かそれ以下

view コンセプトは意味要件が重要であり、構文要件を満たすがコンセプトを満たさない型も少なくありません。そのため view コンセプトを満たすには、view_base を基底クラスにするか enable_view を特殊化することで、明示的に view であることを示す必要があります。

```cpp
template<class T>
  inline constexpr bool enable_view =
    derived_from<T, view_base> || is-derived-from-view-interface<T>;
```

[^view]: [[range.view]](https://eel.is/c++draft/range.view)
