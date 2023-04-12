// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
//===---------------------------------------------------------------------===//

#ifndef _LIBCPP_EXTENTS
#define _LIBCPP_EXTENTS

/*
    extents synopsis

namespace std {
  template<class IndexType, size_t... Extents>
  class extents {
  public:
    using index_type = IndexType;
    using size_type = make_unsigned_t<index_type>;
    using rank_type = size_t;

    // [mdspan.extents.obs], observers of the multidimensional index space
    static constexpr rank_type rank() noexcept { return sizeof...(Extents); }
    static constexpr rank_type rank_dynamic() noexcept { return dynamic-index(rank()); }
    static constexpr size_t static_extent(rank_type) noexcept;
    constexpr index_type extent(rank_type) const noexcept;

    // [mdspan.extents.cons], constructors
    constexpr extents() noexcept = default;

    template<class OtherIndexType, size_t... OtherExtents>
      constexpr explicit(see below)
        extents(const extents<OtherIndexType, OtherExtents...>&) noexcept;
    template<class... OtherIndexTypes>
      constexpr explicit extents(OtherIndexTypes...) noexcept;
    template<class OtherIndexType, size_t N>
      constexpr explicit(N != rank_dynamic())
        extents(span<OtherIndexType, N>) noexcept;
    template<class OtherIndexType, size_t N>
      constexpr explicit(N != rank_dynamic())
        extents(const array<OtherIndexType, N>&) noexcept;

    // [mdspan.extents.cmp], comparison operators
    template<class OtherIndexType, size_t... OtherExtents>
      friend constexpr bool operator==(const extents&,
                                       const extents<OtherIndexType, OtherExtents...>&) noexcept;

    // [mdspan.extents.expo], exposition-only helpers
    constexpr size_t fwd-prod-of-extents(rank_type) const noexcept;     // exposition only
    constexpr size_t rev-prod-of-extents(rank_type) const noexcept;     // exposition only
    template<class OtherIndexType>
      static constexpr auto index-cast(OtherIndexType&&) noexcept;      // exposition only

  private:
    static constexpr rank_type dynamic-index(rank_type) noexcept;       // exposition only
    static constexpr rank_type dynamic-index-inv(rank_type) noexcept;   // exposition only
    array<index_type, rank_dynamic()> dynamic-extents{};                // exposition only
  };

  template<class... Integrals>
    explicit extents(Integrals...)
      -> see below;
}
*/

#include <array>
#include <cinttypes>
#include <limits>
#include <span>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER > 20

namespace detail {

// ------------------------------------------------------------------
// ------------ static_array ----------------------------------------
// ------------------------------------------------------------------
// array like class which provides an array of static values with get

// Implementation of Static Array with recursive implementation of get.
template <size_t R, class T, T... Extents>
struct static_array_impl;

template <size_t R, class T, T FirstExt, T... Extents>
struct static_array_impl<R, T, FirstExt, Extents...> {
  constexpr static T get(size_t r) noexcept {
    if (r == R)
      return FirstExt;
    else
      return static_array_impl<R + 1, T, Extents...>::get(r);
  }
  template <size_t r>
  constexpr static T get() {
    if constexpr (r == R)
      return FirstExt;
    else
      return static_array_impl<R + 1, T, Extents...>::template get<r>();
  }
};

// End the recursion
template <size_t R, class T, T FirstExt>
struct static_array_impl<R, T, FirstExt> {
  constexpr static T get(size_t) noexcept { return FirstExt; }
  template <size_t>
  constexpr static T get() {
    return FirstExt;
  }
};

// Don't start recursion if size 0
template <class T>
struct static_array_impl<0, T> {
  constexpr static T get(size_t) noexcept { return T(); }
  template <size_t>
  constexpr static T get() {
    return T();
  }
};

// Static array, provides get<r>(), get(r) and operator[r]
template <class T, T... Values>
struct static_array : public static_array_impl<0, T, Values...> {
public:
  using value_type = T;

  constexpr static size_t size() { return sizeof...(Values); }
};

// ------------------------------------------------------------------
// ------------ index_sequence_scan ---------------------------------
// ------------------------------------------------------------------

// index_sequence_scan takes compile time values and provides get(r)
//  which return the sum of the first r-1 values.

// Recursive implementation for get
template <size_t R, size_t... Values>
struct index_sequence_scan_impl;

template <size_t R, size_t FirstVal, size_t... Values>
struct index_sequence_scan_impl<R, FirstVal, Values...> {
  constexpr static size_t get(size_t r) {
    if (r > R)
      return FirstVal + index_sequence_scan_impl<R + 1, Values...>::get(r);
    else
      return 0;
  }
};

template <size_t R, size_t FirstVal>
struct index_sequence_scan_impl<R, FirstVal> {
#  if defined(__NVCC__) || defined(__NVCOMPILER)
  // NVCC warns about pointless comparison with 0 for R==0 and r being const
  // evaluatable and also 0.
  constexpr static size_t get(size_t r) { return static_cast<int64_t>(R) > static_cast<int64_t>(r) ? FirstVal : 0; }
#  else
  constexpr static size_t get(size_t r) { return R > r ? FirstVal : 0; }
#  endif
};
template <>
struct index_sequence_scan_impl<0> {
  constexpr static size_t get(size_t) { return 0; }
};

// ------------------------------------------------------------------
// ------------ possibly_empty_array  -------------------------------
// ------------------------------------------------------------------

// array like class which provides get function and operator [], and
// has a specialization for the size 0 case.
// This is needed to make the maybe_static_array be truly empty, for
// all static values.

template <class T, size_t N>
struct possibly_empty_array {
  T vals[N];
  constexpr T& operator[](size_t r) { return vals[r]; }
  constexpr const T& operator[](size_t r) const { return vals[r]; }
};

template <class T>
struct possibly_empty_array<T, 0> {
  constexpr T operator[](size_t) { return T(); }
  constexpr const T operator[](size_t) const { return T(); }
};

// ------------------------------------------------------------------
// ------------ maybe_static_array ----------------------------------
// ------------------------------------------------------------------

// array like class which has a mix of static and runtime values but
// only stores the runtime values.
// The type of the static and the runtime values can be different.
// The position of a dynamic value is indicated through a tag value.
template <class TDynamic, class TStatic, TStatic dyn_tag, TStatic... Values>
struct maybe_static_array {
  static_assert(is_convertible<TStatic, TDynamic>::value,
                "maybe_static_array: TStatic must be convertible to TDynamic");
  static_assert(is_convertible<TDynamic, TStatic>::value,
                "maybe_static_array: TDynamic must be convertible to TStatic");

private:
  // Static values member
  using static_vals_t                    = static_array<TStatic, Values...>;
  constexpr static size_t m_size         = sizeof...(Values);
  constexpr static size_t m_size_dynamic = ((Values == dyn_tag) + ... + 0);

  // Dynamic values member
  [[no_unique_address]] possibly_empty_array<TDynamic, m_size_dynamic> m_dyn_vals;

  // static mapping of indices to the position in the dynamic values array
  using dyn_map_t = index_sequence_scan_impl<0, static_cast<size_t>(Values == dyn_tag)...>;

public:
  // two types for static and dynamic values
  using value_type        = TDynamic;
  using static_value_type = TStatic;
  // tag value indicating dynamic value
  constexpr static static_value_type tag_value = dyn_tag;

  constexpr maybe_static_array() = default;

  // constructor for all static values
  // TODO: add precondition check?
  template <class... Vals>
    requires((m_size_dynamic == 0) && (sizeof...(Vals) > 0))
  constexpr maybe_static_array(Vals...) : m_dyn_vals{} {}

  // constructors from dynamic values only
  template <class... DynVals>
    requires(sizeof...(DynVals) == m_size_dynamic && m_size_dynamic > 0)
  constexpr maybe_static_array(DynVals... vals) : m_dyn_vals{static_cast<TDynamic>(vals)...} {}

  template <class T, size_t N>
    requires(N == m_size_dynamic && N > 0)
  constexpr maybe_static_array(const std::array<T, N>& vals) {
    for (size_t r = 0; r < N; r++)
      m_dyn_vals[r] = static_cast<TDynamic>(vals[r]);
  }

  template <class T, size_t N>
    requires(N == m_size_dynamic && N == 0)
  constexpr maybe_static_array(const std::array<T, N>&) : m_dyn_vals{} {}

  template <class T, size_t N>
    requires(N == m_size_dynamic && N > 0)
  constexpr maybe_static_array(const std::span<T, N>& vals) {
    for (size_t r = 0; r < N; r++)
      m_dyn_vals[r] = static_cast<TDynamic>(vals[r]);
  }

  template <class T, size_t N>
    requires(N == m_size_dynamic && N == 0)
  constexpr maybe_static_array(const std::span<T, N>&) : m_dyn_vals{} {}

  // constructors from all values
  template <class... DynVals>
    requires(sizeof...(DynVals) != m_size_dynamic && m_size_dynamic > 0)
  constexpr maybe_static_array(DynVals... vals) {
    static_assert((sizeof...(DynVals) == m_size), "Invalid number of values.");
    TDynamic values[m_size]{static_cast<TDynamic>(vals)...};
    for (size_t r = 0; r < m_size; r++) {
      TStatic static_val = static_vals_t::get(r);
      if (static_val == dyn_tag) {
        m_dyn_vals[dyn_map_t::get(r)] = values[r];
      }
// Precondition check
#  ifdef _MDSPAN_DEBUG
      else {
        assert(values[r] == static_cast<TDynamic>(static_val));
      }
#  endif
    }
  }

  template <class T, size_t N>
    requires(N != m_size_dynamic && m_size_dynamic > 0)
  constexpr maybe_static_array(const std::array<T, N>& vals) {
    static_assert((N == m_size), "Invalid number of values.");
// Precondition check
#  ifdef _MDSPAN_DEBUG
    assert(N == m_size);
#  endif
    for (size_t r = 0; r < m_size; r++) {
      TStatic static_val = static_vals_t::get(r);
      if (static_val == dyn_tag) {
        m_dyn_vals[dyn_map_t::get(r)] = static_cast<TDynamic>(vals[r]);
      }
// Precondition check
#  ifdef _MDSPAN_DEBUG
      else {
        assert(static_cast<TDynamic>(vals[r]) == static_cast<TDynamic>(static_val));
      }
#  endif
    }
  }

  template <class T, size_t N>
    requires(N != m_size_dynamic && m_size_dynamic > 0)
  constexpr maybe_static_array(const std::span<T, N>& vals) {
    static_assert((N == m_size) || (m_size == dynamic_extent));
#  ifdef _MDSPAN_DEBUG
    assert(N == m_size);
#  endif
    for (size_t r = 0; r < m_size; r++) {
      TStatic static_val = static_vals_t::get(r);
      if (static_val == dyn_tag) {
        m_dyn_vals[dyn_map_t::get(r)] = static_cast<TDynamic>(vals[r]);
      }
#  ifdef _MDSPAN_DEBUG
      else {
        assert(static_cast<TDynamic>(vals[r]) == static_cast<TDynamic>(static_val));
      }
#  endif
    }
  }

  // access functions
  constexpr static TStatic static_value(size_t r) noexcept { return static_vals_t::get(r); }

  constexpr TDynamic value(size_t r) const {
    TStatic static_val = static_vals_t::get(r);
    return static_val == dyn_tag ? m_dyn_vals[dyn_map_t::get(r)] : static_cast<TDynamic>(static_val);
  }
  constexpr TDynamic operator[](size_t r) const { return value(r); }

  // observers
  constexpr static size_t size() { return m_size; }
  constexpr static size_t size_dynamic() { return m_size_dynamic; }
};

} // namespace detail

// ------------------------------------------------------------------
// ------------ extents ---------------------------------------------
// ------------------------------------------------------------------

// Class to describe the extents of a multi dimensional array.
// Used by mdspan, mdarray and layout mappings.
// See ISO C++ standard [mdspan.extents]

template <class IndexType, size_t... Extents>
class extents {
public:
  // typedefs for integral types used
  using index_type = IndexType;
  using size_type  = make_unsigned_t<index_type>;
  using rank_type  = size_t;

  static_assert(std::is_integral<index_type>::value && !std::is_same<index_type, bool>::value,
                "extents::index_type must be a signed or unsigned integer type");

private:
  constexpr static rank_type m_rank         = sizeof...(Extents);
  constexpr static rank_type m_rank_dynamic = ((Extents == dynamic_extent) + ... + 0);

  // internal storage type using maybe_static_array
  using vals_t = detail::maybe_static_array<IndexType, size_t, dynamic_extent, Extents...>;
  [[no_unique_address]] vals_t m_vals;

public:
  // [mdspan.extents.obs], observers of multidimensional index space
  constexpr static rank_type rank() noexcept { return m_rank; }
  constexpr static rank_type rank_dynamic() noexcept { return m_rank_dynamic; }

  constexpr index_type extent(rank_type r) const noexcept { return m_vals.value(r); }
  constexpr static size_t static_extent(rank_type r) noexcept { return vals_t::static_value(r); }

  // [mdspan.extents.cons], constructors
  constexpr extents() noexcept = default;

  // Construction from just dynamic or all values.
  // Precondition check is deferred to maybe_static_array constructor
  template <class... OtherIndexTypes>
    requires((is_convertible_v<OtherIndexTypes, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, OtherIndexTypes> && ...) &&
             (sizeof...(OtherIndexTypes) == m_rank || sizeof...(OtherIndexTypes) == m_rank_dynamic))
  constexpr explicit extents(OtherIndexTypes... dynvals) noexcept : m_vals(static_cast<index_type>(dynvals)...) {}

  template <class OtherIndexType, size_t N>
    requires(is_convertible_v<OtherIndexType, index_type>&& is_nothrow_constructible_v<index_type, OtherIndexType> &&
             (N == m_rank || N == m_rank_dynamic))
  explicit(N != m_rank_dynamic) constexpr extents(const array<OtherIndexType, N>& exts) noexcept
      : m_vals(std::move(exts)) {}

  template <class OtherIndexType, size_t N>
    requires(is_convertible_v<OtherIndexType, index_type>&& is_nothrow_constructible_v<index_type, OtherIndexType> &&
             (N == m_rank || N == m_rank_dynamic))
  explicit(N != m_rank_dynamic) constexpr extents(const span<OtherIndexType, N>& exts) noexcept
      : m_vals(std::move(exts)) {}

private:
  // Function to construct extents storage from other extents.
  // With C++ 17 the first two variants could be collapsed using if constexpr
  // in which case you don't need all the requires clauses.
  // in C++ 14 mode that doesn't work due to infinite recursion
  template <size_t DynCount, size_t R, class OtherExtents, class... DynamicValues>
    requires((R < m_rank) && (static_extent(R) == dynamic_extent))
  vals_t __construct_vals_from_extents(
      std::integral_constant<size_t, DynCount>,
      std::integral_constant<size_t, R>,
      const OtherExtents& exts,
      DynamicValues... dynamic_values)
  noexcept {
    return __construct_vals_from_extents(
        std::integral_constant<size_t, DynCount + 1>(),
        std::integral_constant<size_t, R + 1>(),
        exts,
        dynamic_values...,
        exts.extent(R));
  }

  template <size_t DynCount, size_t R, class OtherExtents, class... DynamicValues>
    requires((R < m_rank) && (static_extent(R) != dynamic_extent))
  vals_t __construct_vals_from_extents(
      std::integral_constant<size_t, DynCount>,
      std::integral_constant<size_t, R>,
      const OtherExtents& exts,
      DynamicValues... dynamic_values)
  noexcept {
    return __construct_vals_from_extents(
        std::integral_constant<size_t, DynCount>(), std::integral_constant<size_t, R + 1>(), exts, dynamic_values...);
  }

  template <size_t DynCount, size_t R, class OtherExtents, class... DynamicValues>
    requires((R == m_rank) && (DynCount == m_rank_dynamic))
  vals_t __construct_vals_from_extents(
      std::integral_constant<size_t, DynCount>,
      std::integral_constant<size_t, R>,
      const OtherExtents&,
      DynamicValues... dynamic_values)
  noexcept { return vals_t{static_cast<index_type>(dynamic_values)...}; }

public:
  // Converting constructor from other extents specializations
  template <class OtherIndexType, size_t... OtherExtents>
    requires((sizeof...(OtherExtents) == sizeof...(Extents)) &&
             ((OtherExtents == dynamic_extent || Extents == dynamic_extent || OtherExtents == Extents) && ...))
  explicit(
      (((Extents != dynamic_extent) && (OtherExtents == dynamic_extent)) || ...) ||
      (std::numeric_limits<index_type>::max() <
       std::numeric_limits<OtherIndexType>::max())) constexpr extents(const extents<OtherIndexType, OtherExtents...>&
                                                                          other) noexcept
      : m_vals(__construct_vals_from_extents(
            std::integral_constant<size_t, 0>(), std::integral_constant<size_t, 0>(), other)) {}

  // Comparison operator
  template <class OtherIndexType, size_t... OtherExtents>
  friend constexpr bool operator==(const extents& lhs, const extents<OtherIndexType, OtherExtents...>& rhs) noexcept {
    bool value = true;
    if constexpr (rank() != sizeof...(OtherExtents)) {
      value = false;
    } else {
      for (size_type r = 0; r < m_rank; r++)
        value &= static_cast<index_type>(rhs.extent(r)) == lhs.extent(r);
    }
    return value;
  }

  template <class OtherIndexType, size_t... OtherExtents>
  friend constexpr bool operator!=(extents const& lhs, extents<OtherIndexType, OtherExtents...> const& rhs) noexcept {
    return !(lhs == rhs);
  }
};

// Recursive helper classes to implement dextents alias for extents
namespace detail {

template <class IndexType, size_t Rank, class Extents = extents<IndexType>>
struct __make_dextents;

template <class IndexType, size_t Rank, size_t... ExtentsPack>
struct __make_dextents< IndexType, Rank, extents<IndexType, ExtentsPack...>> {
  using type = typename __make_dextents< IndexType, Rank - 1, extents<IndexType, dynamic_extent, ExtentsPack...>>::type;
};

template <class IndexType, size_t... ExtentsPack>
struct __make_dextents< IndexType, 0, extents<IndexType, ExtentsPack...>> {
  using type = extents<IndexType, ExtentsPack...>;
};

} // end namespace detail

// [mdspan.extents.dextents], alias template
template <class IndexType, size_t Rank>
using dextents = typename detail::__make_dextents<IndexType, Rank>::type;

// Deduction guide for extents
template <class... IndexTypes>
extents(IndexTypes...) -> extents<size_t, size_t((IndexTypes(), dynamic_extent))...>;

// Helper type traits for identifying a class as extents.
namespace detail {

template <class T>
struct __is_extents : ::std::false_type {};

template <class IndexType, size_t... ExtentsPack>
struct __is_extents<extents<IndexType, ExtentsPack...>> : ::std::true_type {};

template <class T>
inline constexpr bool __is_extents_v = __is_extents<T>::value;

} // namespace detail

#endif // _LIBCPP_STD_VER > 20

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_EXTENTS
