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

template<class E, class ... Args>
constexpr void test_stride(std::array<typename E::index_type, E::rank()> strides, Args ... args) {
  using M = std::layout_right::mapping<E>;
  M m(E(args...));

  for(size_t r=0; r < E::rank(); r++)
    assert(strides[r] == m.stride(r));
}

constexpr bool test() {
  constexpr size_t D = std::dynamic_extent;
  test_stride<std::extents<int>>(std::array<int,0>{});
  test_stride<std::extents<unsigned, D>>(std::array<unsigned, 1>{1}, 7);
  test_stride<std::extents<unsigned, 7>>(std::array<unsigned, 1>{1});
  test_stride<std::extents<unsigned, 7, 8>>(std::array<unsigned, 2>{8, 1});
  test_stride<std::extents<int64_t, D, 8, D, D>>(std::array<int64_t, 4>{720, 90, 10, 1}, 7, 9, 10);
  return true;
}

int main() {
  test();
  static_assert(test());
}
