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

// std::extents can be constructed from just indices, a std::array, or a std::span
// In each of those cases one can either provide all extents, or just the dynamic ones
// If constructed from std::span, the span needs to have a static extent
// Furthermore, the indices/array/span can have integer types other than index_type

template <class E, class AllExtents>
void test_runtime_observers(E ext, AllExtents expected) {
  for (typename E::rank_type r = 0; r < ext.rank(); r++) {
    ASSERT_SAME_TYPE(decltype(ext.extent(0)), typename E::index_type);
    assert(ext.extent(r) == static_cast<typename E::index_type>(expected[r]));
  }
}

template <class E, class AllExtents>
void test_implicit_construction_call(E e, AllExtents all_ext) {
  test_runtime_observers(e, all_ext);
}

template <class E, class AllExtents, class Extents>
void test_implicit_construction(AllExtents all_ext, Extents ext) {
  test_implicit_construction_call<E>(ext, all_ext);
  test_implicit_construction_call<E>(
      std::span<typename Extents::value_type, E::rank_dynamic()>(ext.data(), ext.size()), all_ext);
}

template <class E, class AllExtents, class Extents, size_t... Indices>
void test_construction(AllExtents all_ext, Extents ext, std::index_sequence<Indices...>) {
  // construction from indices
  test_runtime_observers(E(ext[Indices]...), all_ext);
  // construction from array
  test_runtime_observers(E(ext), all_ext);
  // construction from span
  test_runtime_observers(
      E(std::span<typename Extents::value_type, sizeof...(Indices)>(ext.data(), sizeof...(Indices))), all_ext);
}

template <class E, class AllExtents>
void test_construction(AllExtents all_ext) {
  // test construction from all extents
  test_construction<E>(all_ext, all_ext, std::make_index_sequence<E::rank()>());

  // test construction from just dynamic extents
  // create an array of just the extents corresponding to dynamic values
  std::array<typename AllExtents::value_type, E::rank_dynamic()> dyn_ext;
  size_t dynamic_idx = 0;
  for (size_t r = 0; r < E::rank(); r++) {
    if (E::static_extent(r) == std::dynamic_extent) {
      dyn_ext[dynamic_idx] = all_ext[r];
      dynamic_idx++;
    }
  }
  test_construction<E>(all_ext, dyn_ext, std::make_index_sequence<E::rank_dynamic()>());
  test_implicit_construction<E>(all_ext, dyn_ext);
}

template <class T, class TArg>
void test() {
  constexpr size_t D = std::dynamic_extent;

  test_construction<std::extents<T>>(std::array<TArg, 0>{});

  test_construction<std::extents<T, 3>>(std::array<TArg, 1>{3});
  test_construction<std::extents<T, D>>(std::array<TArg, 1>{3});

  test_construction<std::extents<T, 3, 7>>(std::array<TArg, 2>{3, 7});
  test_construction<std::extents<T, 3, D>>(std::array<TArg, 2>{3, 7});
  test_construction<std::extents<T, D, 7>>(std::array<TArg, 2>{3, 7});
  test_construction<std::extents<T, D, D>>(std::array<TArg, 2>{3, 7});

  test_construction<std::extents<T, 3, 7, 9>>(std::array<TArg, 3>{3, 7, 9});
  test_construction<std::extents<T, 3, 7, D>>(std::array<TArg, 3>{3, 7, 9});
  test_construction<std::extents<T, 3, D, D>>(std::array<TArg, 3>{3, 7, 9});
  test_construction<std::extents<T, D, 7, D>>(std::array<TArg, 3>{3, 7, 9});
  test_construction<std::extents<T, D, D, D>>(std::array<TArg, 3>{3, 7, 9});
  test_construction<std::extents<T, 3, D, 9>>(std::array<TArg, 3>{3, 7, 9});
  test_construction<std::extents<T, D, D, 9>>(std::array<TArg, 3>{3, 7, 9});
  test_construction<std::extents<T, D, 7, 9>>(std::array<TArg, 3>{3, 7, 9});

  test_construction<std::extents<T, 1, 2, 3, 4, 5, 6, 7, 8, 9>>(std::array<TArg, 9>{1, 2, 3, 4, 5, 6, 7, 8, 9});
  test_construction<std::extents<T, D, 2, 3, D, 5, D, 7, D, 9>>(std::array<TArg, 9>{1, 2, 3, 4, 5, 6, 7, 8, 9});
  test_construction<std::extents<T, D, D, D, D, D, D, D, D, D>>(std::array<TArg, 9>{1, 2, 3, 4, 5, 6, 7, 8, 9});
}

int main() {
  test<int, int>();
  test<int, size_t>();
  test<unsigned, int>();
  test<char, size_t>();
  test<long long, unsigned>();
  test<size_t, int>();
  test<size_t, size_t>();
  test<int, IntType>();
  test<unsigned char, IntType>();
}
