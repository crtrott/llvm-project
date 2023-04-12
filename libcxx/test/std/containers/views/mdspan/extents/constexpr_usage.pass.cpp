#include <mdspan>
#include <type_traits>
#include <concepts>
#include <cassert>

#include "ConvertibleToIntegral.h"
#include "test_macros.h"

// std::extents can be constructed from just indices, a std::array, or a std::span
// In each of those cases one can either provide all extents, or just the dynamic ones
// If constructed from std::span, the span needs to have a static extent
// Furthermore, the indices/array/span can have integer types other than index_type

// All of this should work in constant expressions

// This tests the actual observers and returns the sum of extents + rank
template <class E, class AllExtents>
constexpr size_t test_runtime_observers(E ext, AllExtents expected) {
  size_t result = 0;
  for (typename E::rank_type r = 0; r < ext.rank(); r++) {
    result += ext.extent(r) == static_cast<typename E::index_type>(expected[r]);
    result += ext.extent(r);
  }
  return result;
}

// Make sure constexpr usage of extents works in implicit construction
template <class E, class AllExtents>
constexpr size_t test_implicit_construction_call(E e, AllExtents all_ext) {
  return test_runtime_observers(e, all_ext);
}

template <class E, class AllExtents, class Extents>
constexpr size_t test_implicit_construction(AllExtents all_ext, Extents ext) {
  return test_implicit_construction_call<E>(ext, all_ext) +
         test_implicit_construction_call<E>(
             std::span<typename Extents::value_type, E::rank_dynamic()>(ext.data(), ext.size()), all_ext);
}

template <class E, class AllExtents, class Extents, size_t... Indices>
constexpr size_t test_construction(AllExtents all_ext, Extents ext, std::index_sequence<Indices...>) {
  size_t result = 0;
  // construction from indicies
  result += test_runtime_observers(E(ext[Indices]...), all_ext);
  // construction from array
  result += test_runtime_observers(E(ext), all_ext);
  // construction from span
  result += test_runtime_observers(
      E(std::span<typename Extents::value_type, sizeof...(Indices)>(ext.data(), sizeof...(Indices))), all_ext);
  return result;
}

template <class E, class AllExtents>
constexpr size_t test_construction(AllExtents all_ext) {
  size_t result = 0;
  // test construction from all extents
  result += test_construction<E>(all_ext, all_ext, std::make_index_sequence<E::rank()>());

  // test construction from just dynamic extents
  // create an array of just the extents corresponding to dynamic values
  std::array<typename AllExtents::value_type, E::rank_dynamic()> dyn_ext{0};
  size_t dynamic_idx = 0;
  for (size_t r = 0; r < E::rank(); r++) {
    if (E::static_extent(r) == std::dynamic_extent) {
      dyn_ext[dynamic_idx] = all_ext[r];
      dynamic_idx++;
    }
  }
  result += test_construction<E>(all_ext, dyn_ext, std::make_index_sequence<E::rank_dynamic()>());
  result += test_implicit_construction<E>(all_ext, dyn_ext);
  return result;
}

template <size_t result, size_t expected>
struct const_expr {
  // num_tests is how many different construction scenarios each
  // construction tests expands to in the end.
  // expected is the value each individual test return
  constexpr static size_t num_tests = 8;
  constexpr static bool value       = result == num_tests * expected;
};

template <class T, class TArg>
void test() {
  constexpr size_t D = std::dynamic_extent;

  static_assert(const_expr<test_construction<std::extents<T>>(std::array<TArg, 0>{}), 0>::value);

  static_assert(const_expr<test_construction<std::extents<T, 3>>(std::array<TArg, 1>{3}), 3 + 1>::value);

  static_assert(const_expr<test_construction<std::extents<T, D>>(std::array<TArg, 1>{3}), 3 + 1>::value);

  static_assert(const_expr<test_construction<std::extents<T, 3, 7>>(std::array<TArg, 2>{3, 7}), 10 + 2>::value);
  static_assert(const_expr<test_construction<std::extents<T, 3, D>>(std::array<TArg, 2>{3, 7}), 10 + 2>::value);
  static_assert(const_expr<test_construction<std::extents<T, D, 7>>(std::array<TArg, 2>{3, 7}), 10 + 2>::value);
  static_assert(const_expr<test_construction<std::extents<T, D, D>>(std::array<TArg, 2>{3, 7}), 10 + 2>::value);

  static_assert(const_expr<test_construction<std::extents<T, 3, 7, 9>>(std::array<TArg, 3>{3, 7, 9}), 19 + 3>::value);
  static_assert(const_expr<test_construction<std::extents<T, 3, 7, D>>(std::array<TArg, 3>{3, 7, 9}), 19 + 3>::value);
  static_assert(const_expr<test_construction<std::extents<T, 3, D, D>>(std::array<TArg, 3>{3, 7, 9}), 19 + 3>::value);
  static_assert(const_expr<test_construction<std::extents<T, D, 7, D>>(std::array<TArg, 3>{3, 7, 9}), 19 + 3>::value);
  static_assert(const_expr<test_construction<std::extents<T, D, D, D>>(std::array<TArg, 3>{3, 7, 9}), 19 + 3>::value);
  static_assert(const_expr<test_construction<std::extents<T, 3, D, 9>>(std::array<TArg, 3>{3, 7, 9}), 19 + 3>::value);
  static_assert(const_expr<test_construction<std::extents<T, D, D, 9>>(std::array<TArg, 3>{3, 7, 9}), 19 + 3>::value);
  static_assert(const_expr<test_construction<std::extents<T, D, 7, 9>>(std::array<TArg, 3>{3, 7, 9}), 19 + 3>::value);

  static_assert(const_expr<test_construction<std::extents<T, 1, 2, 3, 4, 5, 6, 7, 8, 9>>(
                               std::array<TArg, 9>{1, 2, 3, 4, 5, 6, 7, 8, 9}),
                           45 + 9>::value);
  static_assert(const_expr<test_construction<std::extents<T, D, 2, 3, D, 5, D, 7, D, 9>>(
                               std::array<TArg, 9>{1, 2, 3, 4, 5, 6, 7, 8, 9}),
                           45 + 9>::value);
  static_assert(const_expr<test_construction<std::extents<T, D, D, D, D, D, D, D, D, D>>(
                               std::array<TArg, 9>{1, 2, 3, 4, 5, 6, 7, 8, 9}),
                           45 + 9>::value);
}

int main() {
  test<int, int>();
  test<int, size_t>();
  test<char, size_t>();
  test<int, IntType>();
  test<unsigned char, IntType>();
}
