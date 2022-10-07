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
template <class Op, class I1, class I2>
concept indirectly_binary_invocable =
  indirectly_readable<I1> and
  indirectly_readable<I2> and
  copy_constructible<Op> and
  invocable<Op&, iter_value_t<I1>&, iter_value_t<I2>&> and
  invocable<Op&, iter_value_t<I1>&, iter_reference_t<I2>> and
  invocable<Op&, iter_reference_t<I1>, iter_value_t<I2>&> and
  invocable<Op&, iter_reference_t<I1>, iter_reference_t<I2>> and
  invocable<Op&, iter_common_reference_t<I1>, iter_common_reference_t<I2>> and
  common_reference_with<
    invoke_result_t<Op&, iter_value_t<I1>&, iter_value_t<I2>&>,
    invoke_result_t<Op&, iter_value_t<I1>&, iter_reference_t<I2>>,
    invoke_result_t<Op&, iter_reference_t<I1>, iter_value_t<I2>&>,
    invoke_result_t<Op&, iter_reference_t<I1>, iter_reference_t<I2>>>;

template <input_iterator I, sentinel_for<I> S, movable T,
          class Op = plus<>, class Proj = identity>
requires indirectly_binary_invocable<Op&, T*, projected<I, Proj>> and
  assignable_from<T&, indirect_result_t<Op&, T*, projected<I, Proj>>>
constexpr T accumulate(I first, S last, T init, Op op = {}, Proj proj = {});
```

この型制約でも上手くいくように見えますが、この accumulate は採択されませんでした。

[^range-v3]: [range-v3/accumulate.hpp at master · ericniebler/range-v3](https://github.com/ericniebler/range-v3/blob/689b4f3da769fb21dd7acf62550a038242d832e5/include/range/v3/numeric/accumulate.hpp#L36-L42)

それではなぜ、 `ranges::accumulate` は range-v3 と同様の型制約で採択されなかったのでしょうか。また一方で、`ranges::fold` はなぜ採択されたのでしょうか。

これらの問いは STL におけるコンセプトの設計と密接な関係がありそうです。そこで本稿ではまず、 STL におけるコンセプトの設計思想について説明します。続いてそれに基づいて、`ranges::accumulate` が採択されなかった理由、および `ranges::fold` が採択された理由の説明を試みます。

本稿は規格関連文書の内容に加え、筆者の考察も含まれます。筆者の不勉強のために、事実とは異なる記述がある可能性があります。お気づきの点がございましたら、コメント頂けますと幸いです。

## STL におけるコンセプトの設計

<!-- NOTE: 要注意用語: 要件(requirement)、制約(constraint)、コンセプト、型制約、要求 -->

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

[^n2914]: [N2914 - Working Draft, Standard for Programming Language C++](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2009/n2914.pdf)

STL は両者のバランスをとることを目指しています。すなわち複数のクラスに適用可能な普遍的なアルゴリズムを提供しつつも、各コンセプトから意味が失われないコンセプト設計を目指しています。<!-- TODO: 文構造の反復を解消する -->

### コンセプトの指針

上述の目的を達成するために、STL はコンセプトに強い指針を与えています。それは一言で言うと<!-- TODO: 「一言」よりも適切な言葉を探す -->

$$
Concepts = Constraints + Axioms
$$

と表されます。

ここで _Constraints_ とは、コンパイル時に検査することのできる構文要件 (syntatic requirements) を表します。一方 _Axioms_ とは、実行時に検査することのできる意味要件 (semantic requirements) を意味します。STL は、コンセプトは構文要件と意味要件の両方を有する要件であると設計しています。<!-- TODO: 「設計」よりも適切な言葉を探す -->

STL はコンセプトを意味のある一般的な要件にするために、とりわけ以下の 2 点に注意して設計を行なっています。<!-- TODO: 文章がおかしい -->

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

- **理由**: 制約が小さすぎるため、有意義な意味論を持たせることができない。その結果、_Subtractable_ コンセプトのみ満たすクラスにおいて十分一般性のあるアルゴリズムを提供することができない。
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
template <class T>
  inline constexpr bool enable_view =
    derived_from<T, view_base> || _is-derived-from-view-interface_<T>;
```

[^view]: [[range.view]](https://eel.is/c++draft/range.view)

## range-v3 の accumulate の問題点

STL におけるコンセプト設計を踏まえ、それではなぜ、 `ranges::accumulate` は range-v3 と同様の型制約で採択されなかったのでしょうか。<!-- TODO: 文章がおかしい -->その理由は accumulate という操作が加法と密接に関係している点にあります。

- accumulate というメソッド名
- 従来 `<numeric>` ヘッダにて提供されてきたという歴史的経緯
- 二項演算子を表す関数オブジェクトのデフォルト型が `std::plus<>` である

以上のことを考慮すると、accumulate には数値計算アルゴリズムと同様の要件を課すのが自然になります。STL における数値計算アルゴリズムの要件とは、以下の 2 つです。

1. 被演算子の型 `T`, `U` が共通型であること
   ```cpp
   common_with<T, U>
   ```
2. 演算子の型 `Op` が `T` と `U` の 4 つの組み合わせで呼び出し可能であること (以下、四方呼び出し可能と書きます)
   ```cpp
   invocable<Op, T, T> and invocable<Op, U, U> and
   invocable<Op, T, U> and invocable<Op, U, T>
   ```

### STL の数値計算アルゴリズム要件の背景

これらの要件は数学的な代数構造の定義に由来します。代数構造の定義では、二項演算子の始域および終域は通常一致することが前提となっています。

- **例**: $G$ を集合、$\ast: G\times G\to G$ を $G$ 上の二項演算子とする。このとき、組 $(G, \ast)$ が群であるとは...

しかし、数値計算においては異なる型同士の計算にも意味をもたせることができ、時に有用です。

- **例**: 精度の異なる浮動小数点数型間の計算

異なる型同士の計算を許可しつつも、数学的な代数構造の定義と矛盾しないために、STL の数値計算アルゴリズムでは被演算子の型が共通型であることを要求しています。<!-- TODO: 事実か推測か明記する。事実なら引用元を、推測なら参考文献を付す -->

- **例**: `atan2`, `pow`, `hypot`, `fmax`, `fmin`, `fdim`, `fma`, `gcd`, `lcm`

### accumulate の要件を修正した場合の課題

以上の数値計算アルゴリズムのもつ背景を踏まえると、accumulate の型制約は以下のように修正されます[^p1813]。

```cpp
template <class Op, class T, class U>
concept magma =
  common_with<T, U> and
  regular_invocable<Op, T, T> and
  regular_invocable<Op, U, U> and
  regular_invocable<Op, T, U> and
  regular_invocable<Op, U, T> and
  common_with<invoke_result_t<Op&, T, U>, T> and
  common_with<invoke_result_t<Op&, T, U>, U> and
  same_as<invoke_result_t<Op&, T, U>, invoke_result_t<Op&, U, T>>;

template <class Op, class I1, class I2, class O>
concept indirect_magma =
  indirectly_readable<I1> and
  indirectly_readable<I2> and
  indirectly_writable<O, indirect_result_t<Op&, I1, I2>> and
  magma<Op&, iter_value_t<I1>&, iter_value_t<I2>&> and
  magma<Op&, iter_value_t<I1>&, iter_reference_t<I2>&> and
  magma<Op&, iter_reference_t<I1>, iter_value_t<I2>&> and
  magma<Op&, iter_reference_t<I1>, iter_reference_t<I2>> and
  magma<Op&, iter_common_reference_t<I1>, iter_common_reference_t<I2>>;

template <input_iterator I, sentinel_for<I> S, movable T, class Proj = identity,
          indirect_magma<const T*, projected<I, Proj>, T*> Op = plus<>>
constexpr accumulate_result<I, T>
accumulate(I first, S last, T init, Op op = {}, Proj proj = {});
```

[^p1813]: [P1813R0 A Concept Design for the Numeric Algorithms](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1813r0.pdf)

しかしこの accmulate の型制約は、以下のような有用な操作を不許可としてしまっています。

- **例**:<!-- TODO: 例を書く -->

このことは、accumulate をこれらの操作を包含するより一般的な形で再定義すべきであることを示唆しています。

## fold の改善点

そこで accumulate は型制約はほぼ range-v3 のもののまま、fold として改めて定義されました。fold の型制約は以下のように定義されています。

```cpp
template <class Op, class T, class I, class U>
concept _indirectly-binary-left-foldable-impl_ = // 説明専用
  movable<T> and
  movable<U> and
  convertible_to<T, U> and
  invocable<Op&, U, iter_reference_t<I>> and
  assignable_from<U&, invoke_result_t<Op&, U, iter_reference_t<I>>>;

template <class Op, class T, class I>
concept _indirectly-binary-left-foldable_ = // 説明専用
  copy_constructible<Op> and
  indirectly_readable<I> and
  invocable<Op&, T, iter_reference_t<I>> and
  convertible_to<
    invoke_result_t<Op&, T, iter_reference_t<I>>,
    decay_t<invoke_result_t<Op&, T, iter_reference_t<I>>>> and
  _indirectly-binary-left-foldable-impl_<
    Op, T, I, decay_t<invoke_result_t<Op&, T, iter_reference_t<I>>>>;

template <input_iterator I, sentinel_for<I> S, class T,
          _indirectly-binary-left-foldable_<T, I> Op>
constexpr auto fold_left(I first, S last, T init, Op op);
```

- **改善点**:
  - `fold` という名称に変更され、`Op` のデフォルト型が削除された
    → アルゴリズムを加法という文脈から切り離すことに成功
  - 被演算子の型が共通型であること、および演算子の型が四方呼び出し可能であることを要求しなくなった
    → 上記の例を含めたより一般的なアルゴリズムへと進化した
  - 戻り値の型が `T` から `decay_t<invoke_result_t<Op&, T, iter_reference_t<I>>>` に変更された
    → `assignable_from` の意味要件 (代入後は代入したオブジェクトと値が一致する) を満たす範囲が拡大された

## 範囲比較アルゴリズムとの類似

このような議論 (被演算子の型が共通型であること、および演算子の型が四方呼び出し可能であることを要求するか?) は、範囲比較アルゴリズムの検討ですでに行われていました。

- [P1716R3 `ranges` compare algorithm are over-constrained](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1716r3.html)
- 対象となったアルゴリズム: `find{,_end,_first_of}`, `adjacent_find`, `count`, `mismatch`, `equal`, `search{,_n}`, `replace{,_copy}`, `remove{,_copy}`

これらのアルゴリズムは当初 `relation<Pred&, T, U>` で型制約されていました (すなわち四方呼び出し可能であることが要求されていました)。しかし型制約が過剰であると判断され、`predicate<Pred&, T, U>` に変更されました。

```cpp
template <class F, class... Args>
  concept predicate =
    regular_invocable<F, Args...> and
    _boolean-testable_<invoke_result_t<F, Args...>>;

template <class R, class T, class U>
  concept relation =
    predicate<R, T, T> and predicate<R, U, U> and
    predicate<R, T, U> and predicate<R, U, T>;
```

<!-- TODO: 主題の転換を上手く繋げる -->

STL には異なる 2 つの型が等値比較可能であることを表すコンセプトとして、`eqality_comparable_with` が用意されています。このコンセプトは 2 つの型が共通型であることを要求します。一方、`eqality_comparable_with` を用いて制約された STL のアルゴリズムは現時点ではありません。<!-- 現時点、はいつを指している? -->

```cpp
template <class T, class U>
  concept _weakly-equality-comparable-with_ = // 説明専用
    requires(const remove_reference_t<T>& t,
             const remove_reference_t<U>& u) {
      { t == u } -> _boolean-testable_;
      { t != u } -> _boolean-testable_;
      { u == t } -> _boolean-testable_;
      { u != t } -> _boolean-testable_;
    };

template <class T, class U>
concept equality_comparable_with =
  std::equality_comparable<T> and
  std::equality_comparable<U> and
  std::common_reference_with<
    const std::remove_reference_t<T>&,
    const std::remove_reference_t<U>&> and
  std::equality_comparable<
    std::common_reference_t<
      const std::remove_reference_t<T>&,
      const std::remove_reference_t<U>&>> and
    _weakly-equality-comparable-with_<T, U>;
```

:::message
この定義は C++23 に向けた提案 [P2404R3 Move-only types for equality_comparable_with, totally_ordered_with, and three_way_comparable_with](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2404r3.pdf) にて変更が加えられましたが、本稿では反映できていません。
:::

範囲比較アルゴリズムの型制約と、fold の型制約の類似点は以下の表のようにまとめられます。範囲比較アルゴリズムで採択された変更を考慮すると、accumulate から fold への移行は、前例を踏襲した進化といえそうです。

|        | 範囲比較アルゴリズム                                           | fold                                          |
| ------ | -------------------------------------------------------------- | --------------------------------------------- |
| 不採用 | `equality_comparable_with` <br> 共通型・四方呼び出しを要求する | `magma` <br> 共通型・四方呼び出しを要求する   |
| 採用   | `predicate<Pred&, T, U>` <br> 上記を要求しない                 | `_foldable_<Op&, T, U>` <br> 上記を要求しない |

## おわりに

本稿では accumulate の型制約の正しさについて検討しました。その結果、accumulate は数値型の加法と密接に関係したアルゴリズムと解釈できるため、range-v3 の型制約では妥当ではないことが明らかとなりました。その背景として STL のコンセプトの設計指針、すなわち

- コンセプトは普遍性と有意義な意味論の両者をあわせもつ
- $Concepts = Constraints + Axioms$

が明らかとなりました。

本稿ではコンセプト (_Concepts_) と制約 (_Constraints_) を意味要件の有無の観点から明確に区別して説明しました。しかし規格ではこの意味での制約 (_Constraints_) という語は現れません。一方、コンセプトの構文要件を満たす場合は「コンセプトを満たす」、コンセプトの構文要件と意味要件の両方を満たす場合は「コンセプトのモデルとなる」と区別して表現されており、コンセプト (_Concepts_) と制約 (_Constraints_) の区別は消失したわけではなさそうです。このような移行がコンセプトにおける意味要件の扱いにどのような変化をもたらしたかについては、今後の課題とします。
