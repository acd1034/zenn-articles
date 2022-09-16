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

ここでは例として`enumerate_view`を実装します。
使用例:

```cpp
std::vector<char> v{'a', 'b', 'c'};
for (auto&& [index, value] : enumerate_view(v))
  std::cout << index << ":" << value << ","; // output: 0:a,1:b,2:c,
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

  class iterator;
  class sentinel;

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
  using difference_type = std::ranges::range_difference_t<View>;

  constexpr iterator(std::ranges::iterator_t<View> current, std::size_t count)
    : current_(std::move(current)), count_(std::move(count)) {}
};

template <std::ranges::input_range View>
requires std::ranges::view<View>
struct enumerate_view<View>::sentinel {
private:
  //! 元となる view の終端
  std::ranges::sentinel_t<View> end_ = std::ranges::sentinel_t<View>();

public:
  constexpr explicit sentinel(std::ranges::sentinel_t<View> end) : end_(std::move(end)) {}
};
```
