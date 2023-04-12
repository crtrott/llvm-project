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

#include "ConvertibleToIntegral.h"
#include "test_macros.h"

template <class E>
void implicit_construction(E) {}

template <class T>
void test() {
  constexpr size_t D = std::dynamic_extent;

  // clang-format off
  // test less than rank_dynammic args
  {
    std::array a1{3};
    std::span<int, 1> s1(a1.data(), 1);
    std::extents<T, 1, D, 3, D> e1(2); // expected-error {{no matching constructor for initialization of 'std::extents<unsigned long, 1, D, 3, D>'}}
    std::extents<T, 1, D, 3, D> e2(a1); // expected-error {{no matching constructor for initialization of 'std::extents<unsigned long, 1, D, 3, D>'}}
    std::extents<T, 1, D, 3, D> e3(s1); // expected-error {{no matching constructor for initialization of 'std::extents<unsigned long, 1, D, 3, D>'}}
    (void)e1;
    (void)e2;
    (void)e3;
  }
  // test more than rank_dynammic but less than rank args
  {
    std::array a3{3, 4, 5};
    std::span<int, 3> s3(a3.data(), 3);
    std::extents<T, 1, D, 3, D> e1(3, 4, 5); // expected-error {{no matching constructor for initialization of 'std::extents<unsigned long, 1, D, 3, D>'}}
    std::extents<T, 1, D, 3, D> e2(a3); // expected-error {{no matching constructor for initialization of 'std::extents<unsigned long, 1, D, 3, D>'}}
    std::extents<T, 1, D, 3, D> e3(s3); // expected-error {{no matching constructor for initialization of 'std::extents<unsigned long, 1, D, 3, D>'}}
    (void)e1;
    (void)e2;
    (void)e3;
  }
  // test more than rank args
  {
    std::array a5{3, 4, 5, 6, 7};
    std::span<int, 5> s5(a5.data(), 5);
    std::extents<T, 1, D, 3, D> e1(3, 4, 5, 6, 7); // expected-error {{no matching constructor for initialization of 'std::extents<unsigned long, 1, D, 3, D>'}}
    std::extents<T, 1, D, 3, D> e2(a5); // expected-error {{no matching constructor for initialization of 'std::extents<unsigned long, 1, D, 3, D>'}}
    std::extents<T, 1, D, 3, D> e3(s5); // expected-error {{no matching constructor for initialization of 'std::extents<unsigned long, 1, D, 3, D>'}}
    (void)e1;
    (void)e2;
    (void)e3;
  }

  // test implicit construction fails from span and array if all extents are given
  {
    std::array a5{3, 4, 5, 6, 7};
    std::span<int, 5> s5(a5.data(), 5);
    // check that explicit construction works, i.e. no error
    std::extents<int, D, D, 5, D, D> e1(a5);
    std::extents<int, D, D, 5, D, D> e2(s5);
    (void)e1;
    (void)e2;
    implicit_construction<std::extents<int, D, D, 5, D, D>>(a5); // expected-error {{no matching function for call to 'implicit_construction'}}
    implicit_construction<std::extents<int, D, D, 5, D, D>>(s5); // expected-error {{no matching function for call to 'implicit_construction'}}
  }
  // test construction fails from types not convertible to index_type but convertible to other integer types
  {
     std::array a{IntType(3)};
     std::span<IntType, 1> s{a};
     std::extents<unsigned long, D> e1(IntType(3)); // expected-error {{no matching constructor for initialization of 'std::extents<unsigned long, D>'}}
     std::extents<unsigned long, D> e2(a); // expected-error {{no matching constructor for initialization of 'std::extents<unsigned long, D>'}}
     std::extents<unsigned long, D> e3(s); // expected-error {{no matching constructor for initialization of 'std::extents<unsigned long, D>'}}
     (void)e1;
     (void)e2;
     (void)e3;
  }
  // clang-format on
}

int main() { test<unsigned long>(); }
