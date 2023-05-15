//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <mdspan>

// template<class IndexType, size_t... Extents>
// class extents {
// public:
//  // types
//  using index_type = IndexType;
//  using size_type = make_unsigned_t<index_type>;
//  using rank_type = size_t;
//
//  static constexpr rank_type rank() noexcept { return sizeof...(Extents); }
//  static constexpr rank_type rank_dynamic() noexcept { return dynamic-index(rank()); }
//  ...
//  }

#include <mdspan>
#include <type_traits>
#include <concepts>
#include <cassert>
#include <span>

#include "test_macros.h"


// Common requirements of all layout mappings
template<class M, size_t ... Idxs>
void test_mapping_requirements(std::index_sequence<Idxs...>) {
  using E = typename M::extents_type;
  static_assert(std::__mdspan_detail::__is_extents<E>(), "");
  static_assert(std::is_same_v<typename M::index_type, typename E::index_type>, "");
  static_assert(std::is_same_v<typename M::rank_type, typename E::rank_type>, "");
  static_assert(std::is_same_v<typename M::layout_type::template mapping<E>, M>, "");
  static_assert(std::is_same_v<decltype(std::declval<M>().extents()), const E&>, "");
  static_assert(std::is_same_v<decltype(std::declval<M>()(Idxs...)), typename M::index_type>, "");
  static_assert(std::is_same_v<decltype(std::declval<M>().required_span_size()), typename M::index_type>, "");
  static_assert(std::is_same_v<decltype(std::declval<M>().is_unique()), bool>, "");
  static_assert(std::is_same_v<decltype(std::declval<M>().is_exhaustive()), bool>, "");
  static_assert(std::is_same_v<decltype(std::declval<M>().is_strided()), bool>, "");
  static_assert(std::is_same_v<decltype(std::declval<M>().stride(0)), typename M::index_type>, "");
  static_assert(std::is_same_v<decltype(M::is_always_unique()), bool>, "");
  static_assert(std::is_same_v<decltype(M::is_always_exhaustive()), bool>, "");
  static_assert(std::is_same_v<decltype(M::is_always_strided()), bool>, "");
}


template<class L, class E>
void test_layout_mapping_requirements() {
  using M = typename L::template mapping<E>;
  test_mapping_requirements<M>(std::make_index_sequence<E::rank()>());
}

template<class E>
void test_layout_mapping_right() {
  test_layout_mapping_requirements<std::layout_right, E>();

  using M = std::layout_right::template mapping<E>;
  static_assert(M::is_unique() == true, "");
  static_assert(M::is_exhaustive() == true, "");
  static_assert(M::is_strided() == true, "");
  static_assert(M::is_always_unique() == true, "");
  static_assert(M::is_always_exhaustive() == true, "");
  static_assert(M::is_always_strided() == true, "");

  ASSERT_NOEXCEPT(std::declval<M>().extents());
  ASSERT_NOEXCEPT(std::declval<M>().required_span_size());
  ASSERT_NOEXCEPT(std::declval<M>().is_unique());
  ASSERT_NOEXCEPT(std::declval<M>().is_exhaustive());
  ASSERT_NOEXCEPT(std::declval<M>().is_strided());
  ASSERT_NOEXCEPT(std::declval<M>().stride(0));
  ASSERT_NOEXCEPT(M::is_always_unique());
  ASSERT_NOEXCEPT(M::is_always_exhaustive());
  ASSERT_NOEXCEPT(M::is_always_strided());
}

int main() {
  constexpr size_t D = std::dynamic_extent;
  test_layout_mapping_right<std::extents<int>>();
  test_layout_mapping_right<std::extents<char, 4, 5>>();
  test_layout_mapping_right<std::extents<unsigned, D, 4>>();
  test_layout_mapping_right<std::extents<size_t, D, D, D, D>>();
}
