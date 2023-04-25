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
#include "test_macros.h"

template <class E>
struct implicit_construction {
  bool value;
  implicit_construction(E) : value(true) {}
  template <class T>
  implicit_construction(T) : value(false) {}
};

int main() {
  constexpr size_t D = std::dynamic_extent;
  using E            = std::extents<int, 1, D, 3, D>;

  // check can't construct from too few arguments
  static_assert(!std::is_constructible_v<E, int>, "extents constructible from illegal arguments");
  static_assert(!std::is_constructible_v<E, std::array<int, 1>>, "extents constructible from illegal arguments");
  static_assert(!std::is_constructible_v<E, std::span<int, 1>>, "extents constructible from illegal arguments");
  // check can't construct from rank_dynamic < #args < rank
  static_assert(!std::is_constructible_v<E, int, int, int>, "extents constructible from illegal arguments");
  static_assert(!std::is_constructible_v<E, std::array<int, 3>>, "extents constructible from illegal arguments");
  static_assert(!std::is_constructible_v<E, std::span<int, 3>>, "extents constructible from illegal arguments");
  // check can't construct from too many arguments
  static_assert(!std::is_constructible_v<E, int, int, int, int, int>, "extents constructible from illegal arguments");
  static_assert(!std::is_constructible_v<E, std::array<int, 5>>, "extents constructible from illegal arguments");
  static_assert(!std::is_constructible_v<E, std::span<int, 5>>, "extents constructible from illegal arguments");

  // test implicit construction fails from span and array if all extents are given
  std::array a5{3, 4, 5, 6, 7};
  std::span<int, 5> s5(a5.data(), 5);
  // check that explicit construction works, i.e. no error
  static_assert(std::is_constructible_v< std::extents<int, D, D, 5, D, D>, decltype(a5)>,
                "extents unexpectectly not constructible");
  static_assert(std::is_constructible_v< std::extents<int, D, D, 5, D, D>, decltype(s5)>,
                "extents unexpectectly not constructible");
  // check that implicit construction doesn't work
  LIBCPP_ASSERT((implicit_construction<std::extents<int, D, D, 5, D, D>>(a5).value == false));
  LIBCPP_ASSERT((implicit_construction<std::extents<int, D, D, 5, D, D>>(s5).value == false));

  // test construction fails from types not convertible to index_type but convertible to other integer types
  static_assert(std::is_convertible_v<IntType, int>, "Test helper IntType unexpectedly not convertible to int");
  static_assert(!std::is_constructible_v< std::extents<unsigned long, D>, IntType>,
                "extents constructible from illegal arguments");
  static_assert(!std::is_constructible_v< std::extents<unsigned long, D>, std::array<IntType, 1>>,
                "extents constructible from illegal arguments");
  static_assert(!std::is_constructible_v< std::extents<unsigned long, D>, std::span<IntType, 1>>,
                "extents constructible from illegal arguments");
}
