#include <mdspan>
#include <type_traits>
#include <concepts>
#include <cassert>

#include "test_macros.h"

template <class E, class IndexType, size_t... Extents>
void testExtents() {
  ASSERT_SAME_TYPE(typename E::index_type, IndexType);
  ASSERT_SAME_TYPE(typename E::size_type, std::make_unsigned_t<IndexType>);
  ASSERT_SAME_TYPE(typename E::rank_type, size_t);

  static_assert(sizeof...(Extents) == E::rank());
  static_assert((static_cast<size_t>(Extents == std::dynamic_extent) + ...) == E::rank_dynamic());

  static_assert(std::regular<E>);
  static_assert(std::is_trivially_copyable_v<E>);
}

template <class IndexType, size_t... Extents>
void testExtents() {
  testExtents<std::extents<IndexType, Extents...>, IndexType, Extents...>();
}

template <class T>
void test() {
  constexpr size_t D = std::dynamic_extent;
  testExtents<T, D>();
  testExtents<T, 3>();
  testExtents<T, 3, 3>();
  testExtents<T, 3, D>();
  testExtents<T, D, 3>();
  testExtents<T, D, D>();
  testExtents<T, 3, 3, 3>();
  testExtents<T, 3, 3, D>();
  testExtents<T, 3, D, D>();
  testExtents<T, D, 3, D>();
  testExtents<T, D, D, D>();
  testExtents<T, 3, D, 3>();
  testExtents<T, D, 3, 3>();
  testExtents<T, D, D, 3>();

  testExtents<T, 9, 8, 7, 6, 5, 4, 3, 2, 1>();
  testExtents<T, 9, D, 7, 6, D, D, 3, D, D>();
  testExtents<T, D, D, D, D, D, D, D, D, D>();
}

int main() {
  test<int>();
  test<unsigned>();
  test<char>();
  test<long long>();
  test<size_t>();
}
