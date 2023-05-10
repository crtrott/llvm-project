//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <mdspan>

// Test default iteration:
//
// constexpr extents() noexcept = default;
//
// Remarks: since the standard uses an exposition only array member, dynamic extents
// need to be zero intialized!

#include <mdspan>
#include <cassert>
#include <array>
#include <cstdio>
#include "test_macros.h"

template<class M, class T, class ... Args>
constexpr void iterate_right(M m, T& count, Args ... args) {
  constexpr size_t r = sizeof...(Args);
  if constexpr (M::extents_type::rank() == r) {
    assert(count == m(args...));
    count++;
  } else {
    for(typename M::index_type i = 0; i < m.extents().extent(r); i++) {
      iterate_right(m, count, args ..., i);
    }
  }
}

template<class E, class ... Args>
constexpr void test_iteration(Args ... args) {
  using M = std::layout_right::mapping<E>;
  M m(E(args...));

  typename E::index_type count = 0;
  iterate_right(m, count);
}

constexpr bool test() {
  constexpr size_t D = std::dynamic_extent;
  test_iteration<std::extents<int>>();
  test_iteration<std::extents<unsigned, D>>(7);
  test_iteration<std::extents<unsigned, 7>>();
  test_iteration<std::extents<unsigned, 7, 8>>();
  test_iteration<std::extents<int64_t, D, 8, D, D>>(7, 9, 10);
  return true;
}

int main() {
  test();
  static_assert(test());
}
