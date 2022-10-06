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
<!-- NOTE: Range TS に当初 accumulate は入っていたと思われるので、このbrief historyは誤解を招くかもしれない -->

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
