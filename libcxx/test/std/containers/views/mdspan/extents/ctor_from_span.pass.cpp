//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <mdspan>

// constexpr extents() noexcept;
//
//
// template<class... OtherIndexTypes>
//     constexpr explicit extents(OtherIndexTypes...) noexcept;
//
// Remarks: These constructors shall not participate in overload resolution unless:
//   - (is_convertible_v<OtherIndexTypes, index_type> && ...) is true,
//   - (is_nothrow_constructible_v<index_type, OtherIndexTypes> && ...) is true, and
//   - N == rank_dynamic() || N == rank() is true.
//
//
// template<class OtherIndexType, size_t N>
//     constexpr explicit(N != rank_dynamic()) extents(span<OtherIndexType, N>) noexcept;
// template<class OtherIndexType, size_t N>
//     constexpr explicit(N != rank_dynamic()) extents(const array<OtherIndexType, N>&) noexcept;
//
// Remarks: These constructors shall not participate in overload resolution unless:
//   - is_convertible_v<const OtherIndexType&, index_type> is true,
//   - is_nothrow_constructible_v<index_type, const OtherIndexType&> is true, and
//   - N == rank_dynamic() || N == rank() is true.
//

#include <mdspan>
#include <cassert>
#include <array>
#include <span>

#include "ConvertibleToIntegral.h"
#include "CtorTestCombinations.h"
#include "test_macros.h"

// std::extents can be constructed from just indices, a std::array, or a std::span
// In each of those cases one can either provide all extents, or just the dynamic ones
// If constructed from std::span, the span needs to have a static extent
// Furthermore, the indices/array/span can have integer types other than index_type

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
