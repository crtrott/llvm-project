//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <mdspan>

// Test construction from integral:
//
// template<class ... OtherIndexTypes>
//     constexpr explicit extents(OtherIndexTypes ... exts) noexcept;
//
// Let N be sizeof...(OtherIndexTypes), and let
// exts_arr be array<index_type, N>{static_cast<index_type>(std::move(exts))...}.
//
// Constraints:
//   * (is_convertible_v<OtherIndexTypes, index_type> && ...) is true,
//   * (is_nothrow_constructible_v<index_type, OtherIndexType> && ...) is true, and
//   * N == rank_dynamic() || N == rank() is true.
//
// Preconditions:
//   * If N != rank_dynamic() is true, exts_arr[r] equals Er for each r for which
//     Er is a static extent, and
//   * either
//     - sizeof...(exts) == 0 is true, or
//     - each element of exts is nonnegative and is representable as a value of type index_type.
//

#include <mdspan>
#include <cassert>
#include <array>
#include <span>

#include "ConvertibleToIntegral.h"
#include "CtorTestCombinations.h"
#include "test_macros.h"

struct IntegralCtorTest {
  template <class E, class AllExtents, class Extents, size_t... Indices>
  static constexpr void test_construction(AllExtents all_ext, Extents ext, std::index_sequence<Indices...>) {
    // construction from indices
    ASSERT_NOEXCEPT(E(ext[Indices]...));
    test_runtime_observers(E(ext[Indices]...), all_ext);
  }
};

int main() {
  test_index_type_combo<IntegralCtorTest>();
  static_assert(test_index_type_combo<IntegralCtorTest>());
}
