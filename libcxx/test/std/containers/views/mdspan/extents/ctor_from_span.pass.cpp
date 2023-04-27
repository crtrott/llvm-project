//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <mdspan>

// Test construction from span:
//
// template<class OtherIndexType, size_t N>
//     constexpr explicit(N != rank_dynamic()) extents(span<OtherIndexType, N> exts) noexcept;
//
// Constraints:
//   * is_convertible_v<const OtherIndexType&, index_type> is true,
//   * is_nothrow_constructible_v<index_type, const OtherIndexType&> is true, and
//   * N == rank_dynamic() || N == rank() is true.
//
// Preconditions:
//   * If N != rank_dynamic() is true, exts[r] equals Er for each r for which
//     Er is a static extent, and
//   * either
//     - N is zero, or
//     - exts[r] is nonnegative and is representable as a value of type index_type
//       for every rank index r.
//

#include <mdspan>
#include <cassert>
#include <array>
#include <span>

#include "ConvertibleToIntegral.h"
#include "CtorTestCombinations.h"
#include "test_macros.h"

struct IntegralCtorTest {
  template <class E, class T, size_t N, class Extents, size_t... Indices>
  static constexpr bool test_construction(std::array<T, N> all_ext, Extents ext, std::index_sequence<Indices...>) {
    ASSERT_NOEXCEPT(E(ext));
    if constexpr (N == E::rank_dynamic()) {
      if (!test_implicit_construction_call<E>(std::span(ext), all_ext))
        return false;
    }
    return test_runtime_observers(E(std::span(ext)), all_ext);
  }
};

int main() {
  test_index_type_combo<IntegralCtorTest>();
  static_assert(test_index_type_combo<IntegralCtorTest>());
}
