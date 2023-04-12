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

template <class To, class From>
void test_implicit_conversion(To dest, From src) {
  assert(dest == src);
}

template <bool implicit, class To, class From>
void test_conversion(From src) {
  To dest(src);
  assert(dest == src);
  if constexpr (implicit) {
    dest = src;
    assert(dest == src);
    test_implicit_conversion<To, From>(src, src);
  }
}

template <class T1, class T2>
void test_conversion() {
  constexpr size_t D             = std::dynamic_extent;
  constexpr bool idx_convertible = std::numeric_limits<T1>::max() >= std::numeric_limits<T2>::max();

  // clang-format off
  test_conversion<idx_convertible && true,  std::extents<T1>>(std::extents<T2>());
  test_conversion<idx_convertible && true,  std::extents<T1, D>>(std::extents<T2, D>(5));
  test_conversion<idx_convertible && false, std::extents<T1, 5>>(std::extents<T2, D>(5));
  test_conversion<idx_convertible && true,  std::extents<T1, 5>>(std::extents<T2, 5>());
  test_conversion<idx_convertible && false, std::extents<T1, 5, D>>(std::extents<T2, D, D>(5, 5));
  test_conversion<idx_convertible && true,  std::extents<T1, D, D>>(std::extents<T2, D, D>(5, 5));
  test_conversion<idx_convertible && true,  std::extents<T1, D, D>>(std::extents<T2, D, 7>(5));
  test_conversion<idx_convertible && true,  std::extents<T1, 5, 7>>(std::extents<T2, 5, 7>());
  test_conversion<idx_convertible && false, std::extents<T1, 5, D, 8, D, D>>(std::extents<T2, D, D, 8, 9, 1>(5, 7));
  test_conversion<idx_convertible && true,  std::extents<T1, D, D, D, D, D>>(
                                            std::extents<T2, D, D, D, D, D>(5, 7, 8, 9, 1));
  test_conversion<idx_convertible && true,  std::extents<T1, D, D, 8, 9, D>>(std::extents<T2, D, 7, 8, 9, 1>(5));
  test_conversion<idx_convertible && true,  std::extents<T1, 5, 7, 8, 9, 1>>(std::extents<T2, 5, 7, 8, 9, 1>());
  // clang-format on
}

int main() {
  test_conversion<int, int>();
  test_conversion<int, size_t>();
  test_conversion<size_t, int>();
  test_conversion<size_t, long>();
}
