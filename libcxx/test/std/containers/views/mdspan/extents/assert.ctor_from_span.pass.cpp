//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// REQUIRES: has-unix-headers
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// XFAIL: availability-verbose_abort-missing
// ADDITIONAL_COMPILE_FLAGS: -D_LIBCPP_ENABLE_ASSERTIONS=1

// <mdspan>

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

#include "check_assertion.h"

int main() {
  constexpr size_t D = std::dynamic_extent;
  // working case sanity check
  {
    std::array args{1000, 5};
    [[maybe_unused]] std::extents<int, D, 5> e1(std::span{args});
  }
  // mismatch of static extent
  {
    std::array args{1000, 3};
    TEST_LIBCPP_ASSERT_FAILURE(([=] { std::extents<int, D, 5> e1(std::span{args}); }()),
                               "extents construction: mismatch of provided arguments with static extents.");
  }
  // value out of range
  {
    std::array args{1000, 5};
    TEST_LIBCPP_ASSERT_FAILURE(([=] { std::extents<char, D, 5> e1(std::span{args}); }()),
                               "extents ctor: arguments must be representable as index_type and nonnegative");
  }
  // negative value
  {
    std::array args{-1, 5};
    TEST_LIBCPP_ASSERT_FAILURE(([=] { std::extents<char, D, 5> e1(std::span{args}); }()),
                               "extents ctor: arguments must be representable as index_type and nonnegative");
  }
}
