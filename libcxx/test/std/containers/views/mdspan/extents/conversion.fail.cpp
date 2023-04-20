//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <mdspan>

// template<class OtherIndexType, size_t... OtherExtents>
//     constexpr explicit(see below) extents(const extents<OtherIndexType, OtherExtents...>&) noexcept;
//
// Remarks: These constructors shall not participate in overload resolution unless:
//   - sizeof...(OtherExtents) == rank() is true, and
//   - ((OtherExtents == dynamic_extent || Extents == dynamic_extent || OtherExtents == Extents) && ...) is true.
//

#include <mdspan>
#include <type_traits>
#include <concepts>
#include <cassert>

#include "test_macros.h"

// std::extents can be converted into each other as long as there aren't any
// mismatched static extents.
// Convertibility requires that no runtime dimension is assigned to a static dimension,
// and that the destinations index_type has a larger or equal max value than the
// sources index_type

// clang-format off
void test_no_implicit_conversion() {
  constexpr size_t D = std::dynamic_extent;
  {
    std::extents<int, D> from;
    std::extents<int, 5> to;
    to = from; // expected-error {{no viable overloaded '='}}
  }
  {
    std::extents<int, D, 7> from;
    std::extents<int, 5, 7> to;
    to = from; // expected-error {{no viable overloaded '='}}
  }
  {
    std::extents<size_t, D> from;
    std::extents<int, D> to;
    to = from; // expected-error {{no viable overloaded '='}}
  }
  {
    std::extents<size_t, 5> from;
    std::extents<int, 5> to;
    to = from; // expected-error {{no viable overloaded '='}}
  }
}

void test_rank_mismatch() {
  constexpr size_t D = std::dynamic_extent;
  {
    std::extents<int> from;
    [[maybe_unused]] std::extents<int, D> to(from); // expected-error {{no matching constructor for initialization of 'std::extents<int, D>'}}
  }
  {
    std::extents<int, D, D> from;
    [[maybe_unused]] std::extents<int> to0(from); // expected-error {{no matching constructor for initialization of 'std::extents<int>'}}
    [[maybe_unused]] std::extents<int, D> to1(from); // expected-error {{no matching constructor for initialization of 'std::extents<int, D>'}}
    [[maybe_unused]] std::extents<int, D, D, D> to3(from); // expected-error {{no matching constructor for initialization of 'std::extents<int, D, D, D>'}}
  }
}

void test_static_extent_mismatch() {
  constexpr size_t D = std::dynamic_extent;
  {
    std::extents<int, 3> from;
    [[maybe_unused]] std::extents<int, 2> to(from); // expected-error {{no matching constructor for initialization of 'std::extents<int, 2>'}}
  }
  {
    std::extents<int, 3, D> from;
    [[maybe_unused]] std::extents<int, 2, D> to(from); // expected-error {{no matching constructor for initialization of 'std::extents<int, 2, D>'}}
  }
  {
    std::extents<int, D, 2> from;
    [[maybe_unused]] std::extents<int, D, 3> to(from); // expected-error {{no matching constructor for initialization of 'std::extents<int, D, 3>'}}
  }
}
// clang-format on

int main() {
  test_rank_mismatch();
  test_static_extent_mismatch();
  test_no_implicit_conversion();
}
