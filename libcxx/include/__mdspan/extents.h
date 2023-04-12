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

#ifndef _LIBCPP___MDSPAN_EXTENTS_H
#define _LIBCPP___MDSPAN_EXTENTS_H

/*
    extents synopsis

namespace std {
  template<class _IndexType, size_t... _Extents>
  class extents {
  public:
    using index_type = _IndexType;
    using size_type = make_unsigned_t<index_type>;
    using rank_type = size_t;

    // [mdspan.extents.obs], observers of the multidimensional index space
    static constexpr rank_type rank() noexcept { return sizeof...(_Extents); }
    static constexpr rank_type rank_dynamic() noexcept { return dynamic-index(rank()); }
    static constexpr size_t static_extent(rank_type) noexcept;
    constexpr index_type extent(rank_type) const noexcept;

    // [mdspan.extents.cons], constructors
    constexpr extents() noexcept = default;

    template<class _OtherIndexType, size_t... _OtherExtents>
      constexpr explicit(see below)
        extents(const extents<_OtherIndexType, _OtherExtents...>&) noexcept;
    template<class... _OtherIndexTypes>
      constexpr explicit extents(_OtherIndexTypes...) noexcept;
    template<class _OtherIndexType, size_t N>
      constexpr explicit(N != rank_dynamic())
        extents(span<_OtherIndexType, N>) noexcept;
    template<class _OtherIndexType, size_t N>
      constexpr explicit(N != rank_dynamic())
        extents(const array<_OtherIndexType, N>&) noexcept;

    // [mdspan.extents.cmp], comparison operators
    template<class _OtherIndexType, size_t... _OtherExtents>
      friend constexpr bool operator==(const extents&,
                                       const extents<_OtherIndexType, _OtherExtents...>&) noexcept;

    // [mdspan.extents.expo], exposition-only helpers
    constexpr size_t fwd-prod-of-extents(rank_type) const noexcept;     // exposition only
    constexpr size_t __rev-prod-of-extents(rank_type) const noexcept;     // exposition only
    template<class _OtherIndexType>
      static constexpr auto index-cast(_OtherIndexType&&) noexcept;      // exposition only

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

#if _LIBCPP_STD_VER >= 23

namespace __mdspan_detail {

// ------------------------------------------------------------------
// ------------ __static_array --------------------------------------
// ------------------------------------------------------------------
// array like class which provides an array of static values with get

// Implementation of Static Array with recursive implementation of get.
template <size_t _R, class _T, _T... _Extents>
struct __static_array_impl;

template <size_t _R, class _T, _T _FirstExt, _T... _Extents>
struct __static_array_impl<_R, _T, _FirstExt, _Extents...> {
  constexpr static _T get(size_t __r) noexcept {
    if (__r == _R)
      return _FirstExt;
    else
      return __static_array_impl<_R + 1, _T, _Extents...>::get(__r);
  }
  template <size_t __r>
  constexpr static _T get() {
    if constexpr (__r == _R)
      return _FirstExt;
    else
      return __static_array_impl<_R + 1, _T, _Extents...>::template get<__r>();
  }
};

// End the recursion
template <size_t _R, class _T, _T _FirstExt>
struct __static_array_impl<_R, _T, _FirstExt> {
  constexpr static _T get(size_t) noexcept { return _FirstExt; }
  template <size_t>
  constexpr static _T get() {
    return _FirstExt;
  }
};

// Don't start recursion if size 0
template <class _T>
struct __static_array_impl<0, _T> {
  constexpr static _T get(size_t) noexcept { return _T(); }
  template <size_t>
  constexpr static _T get() {
    return _T();
  }
};

// Static array, provides get<r>(), get(r) and operator[r]
template <class _T, _T... _Values>
struct __static_array : public __static_array_impl<0, _T, _Values...> {
public:
  using value_type = _T;

  constexpr static size_t size() { return sizeof...(_Values); }
};

// ------------------------------------------------------------------
// ------------ index_sequence_scan ---------------------------------
// ------------------------------------------------------------------

// index_sequence_scan takes compile time values and provides get(r)
//  which return the sum of the first r-1 values.

// _Recursive implementation for get
template <size_t _R, size_t... _Values>
struct __index_sequence_scan_impl;

template <size_t _R, size_t _FirstVal, size_t... _Values>
struct __index_sequence_scan_impl<_R, _FirstVal, _Values...> {
  constexpr static size_t get(size_t __r) {
    if (__r > _R)
      return _FirstVal + __index_sequence_scan_impl<_R + 1, _Values...>::get(__r);
    else
      return 0;
  }
};

template <size_t _R, size_t _FirstVal>
struct __index_sequence_scan_impl<_R, _FirstVal> {
#  if defined(__NVCC__) || defined(__NVCOMPILER)
  // NVCC warns about pointless comparison with 0 for _R==0 and r being const
  // evaluatable and also 0.
  constexpr static size_t get(size_t __r) { return static_cast<int64_t>(_R) > static_cast<int64_t>(__r) ? _FirstVal : 0; }
#  else
  constexpr static size_t get(size_t __r) { return _R > __r ? _FirstVal : 0; }
#  endif
};
template <>
struct __index_sequence_scan_impl<0> {
  constexpr static size_t get(size_t) { return 0; }
};

// ------------------------------------------------------------------
// ------------ __possibly_empty_array  -----------------------------
// ------------------------------------------------------------------

// array like class which provides get function and operator [], and
// has a specialization for the size 0 case.
// This is needed to make the __maybe_static_array be truly empty, for
// all static values.

template <class _T, size_t _Num>
struct __possibly_empty_array {
  _T __vals_[_Num];
  constexpr _T& operator[](size_t __r) { return __vals_[__r]; }
  constexpr const _T& operator[](size_t __r) const { return __vals_[__r]; }
};

template <class _T>
struct __possibly_empty_array<_T, 0> {
  constexpr _T operator[](size_t) { return _T(); }
  constexpr const _T operator[](size_t) const { return _T(); }
};

// ------------------------------------------------------------------
// ------------ __maybe_static_array --------------------------------
// ------------------------------------------------------------------

// array like class which has a mix of static and runtime values but
// only stores the runtime values.
// The type of the static and the runtime values can be different.
// The position of a dynamic value is indicated through a tag value.
template <class _TDynamic, class _TStatic, _TStatic _DynTag, _TStatic... _Values>
struct __maybe_static_array {
  static_assert(is_convertible<_TStatic, _TDynamic>::value,
                "__maybe_static_array: _TStatic must be convertible to _TDynamic");
  static_assert(is_convertible<_TDynamic, _TStatic>::value,
                "__maybe_static_array: _TDynamic must be convertible to _TStatic");

private:
  // Static values member
  using __static_vals_t                    = __static_array<_TStatic, _Values...>;
  constexpr static size_t m_size         = sizeof...(_Values);
  constexpr static size_t m_size_dynamic = ((_Values == _DynTag) + ... + 0);

  // Dynamic values member
  [[no_unique_address]] __possibly_empty_array<_TDynamic, m_size_dynamic> __dyn_vals_;

  // static mapping of indices to the position in the dynamic values array
  using __dyn_map_t = __index_sequence_scan_impl<0, static_cast<size_t>(_Values == _DynTag)...>;

public:
  // two types for static and dynamic values
  using value_type        = _TDynamic;
  using static_value_type = _TStatic;
  // tag value indicating dynamic value
  constexpr static static_value_type tag_value = _DynTag;

  constexpr __maybe_static_array() = default;

  // constructor for all static values
  // TODO: add precondition check?
  template <class... _Vals>
    requires((m_size_dynamic == 0) && (sizeof...(_Vals) > 0))
  constexpr __maybe_static_array(_Vals...) : __dyn_vals_{} {}

  // constructors from dynamic values only
  template <class... _DynVals>
    requires(sizeof...(_DynVals) == m_size_dynamic && m_size_dynamic > 0)
  constexpr __maybe_static_array(_DynVals... __vals) : __dyn_vals_{static_cast<_TDynamic>(__vals)...} {}

  template <class _T, size_t _Num>
    requires(_Num == m_size_dynamic && _Num > 0)
  constexpr __maybe_static_array(const std::array<_T, _Num>& __vals) {
    for (size_t __r = 0; __r < _Num; __r++)
      __dyn_vals_[__r] = static_cast<_TDynamic>(__vals[__r]);
  }

  template <class _T, size_t _Num>
    requires(_Num == m_size_dynamic && _Num == 0)
  constexpr __maybe_static_array(const std::array<_T, _Num>&) : __dyn_vals_{} {}

  template <class _T, size_t _Num >
    requires(_Num == m_size_dynamic && _Num > 0)
  constexpr __maybe_static_array(const std::span<_T, _Num>& __vals) {
    for (size_t __r = 0; __r < _Num; __r++)
      __dyn_vals_[__r] = static_cast<_TDynamic>(__vals[__r]);
  }

  template <class _T, size_t _Num>
    requires(_Num == m_size_dynamic && _Num == 0)
  constexpr __maybe_static_array(const std::span<_T, _Num>&) : __dyn_vals_{} {}

  // constructors from all values
  template <class... _DynVals>
    requires(sizeof...(_DynVals) != m_size_dynamic && m_size_dynamic > 0)
  constexpr __maybe_static_array(_DynVals... __vals) {
    static_assert((sizeof...(_DynVals) == m_size), "Invalid number of values.");
    _TDynamic __values[m_size]{static_cast<_TDynamic>(__vals)...};
    for (size_t __r = 0; __r < m_size; __r++) {
      _TStatic __static_val = __static_vals_t::get(__r);
      if (__static_val == _DynTag) {
        __dyn_vals_[__dyn_map_t::get(__r)] = __values[__r];
      }
// Precondition check
#  ifdef _MDSPAN_DEBUG
      else {
        assert(__values[r] == static_cast<_TDynamic>(__static_val));
      }
#  endif
    }
  }

  template <class _T, size_t _Num>
    requires(_Num != m_size_dynamic && m_size_dynamic > 0)
  constexpr __maybe_static_array(const std::array<_T, _Num>& __vals) {
    static_assert((_Num == m_size), "Invalid number of values.");
// Precondition check
#  ifdef _MDSPAN_DEBUG
    assert(_Num == m_size);
#  endif
    for (size_t __r = 0; __r < m_size; __r++) {
      _TStatic __static_val = __static_vals_t::get(__r);
      if (__static_val == _DynTag) {
        __dyn_vals_[__dyn_map_t::get(__r)] = static_cast<_TDynamic>(__vals[__r]);
      }
// Precondition check
#  ifdef _MDSPAN_DEBUG
      else {
        assert(static_cast<_TDynamic>(__vals[__r]) == static_cast<_TDynamic>(__static_val));
      }
#  endif
    }
  }

  template <class _T, size_t _Num>
    requires(_Num != m_size_dynamic && m_size_dynamic > 0)
  constexpr __maybe_static_array(const std::span<_T, _Num>& __vals) {
    static_assert((_Num == m_size) || (m_size == dynamic_extent));
    for (size_t __r = 0; __r < m_size; __r++) {
      _TStatic __static_val = __static_vals_t::get(__r);
      if (__static_val == _DynTag) {
        __dyn_vals_[__dyn_map_t::get(__r)] = static_cast<_TDynamic>(__vals[__r]);
      }
#  ifdef _MDSPAN_DEBUG
      else {
        assert(static_cast<_TDynamic>(__vals[__r]) == static_cast<_TDynamic>(__static_val));
      }
#  endif
    }
  }

  // access functions
  constexpr static _TStatic static_value(size_t __r) noexcept { return __static_vals_t::get(__r); }

  constexpr _TDynamic value(size_t __r) const {
    _TStatic __static_val = __static_vals_t::get(__r);
    return __static_val == _DynTag ? __dyn_vals_[__dyn_map_t::get(__r)] : static_cast<_TDynamic>(__static_val);
  }
  constexpr _TDynamic operator[](size_t __r) const { return value(__r); }

  // observers
  constexpr static size_t size() { return m_size; }
  constexpr static size_t size_dynamic() { return m_size_dynamic; }
};

} // namespace __mdspan_detail

// ------------------------------------------------------------------
// ------------ extents ---------------------------------------------
// ------------------------------------------------------------------

// Class to describe the extents of a multi dimensional array.
// Used by mdspan, mdarray and layout mappings.
// See ISO C++ standard [mdspan.extents]

template <class _IndexType, size_t... _Extents>
class extents {
public:
  // typedefs for integral types used
  using index_type = _IndexType;
  using size_type  = make_unsigned_t<index_type>;
  using rank_type  = size_t;

  static_assert(std::is_integral<index_type>::value && !std::is_same<index_type, bool>::value,
                "extents::index_type must be a signed or unsigned integer type");

private:
  constexpr static rank_type m_rank         = sizeof...(_Extents);
  constexpr static rank_type m_rank_dynamic = ((_Extents == dynamic_extent) + ... + 0);

  // internal storage type using __maybe_static_array
  using vals_t = __mdspan_detail::__maybe_static_array<_IndexType, size_t, dynamic_extent, _Extents...>;
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
  // Precondition check is deferred to __maybe_static_array constructor
  template <class... _OtherIndexTypes>
    requires((is_convertible_v<_OtherIndexTypes, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, _OtherIndexTypes> && ...) &&
             (sizeof...(_OtherIndexTypes) == m_rank || sizeof...(_OtherIndexTypes) == m_rank_dynamic))
  constexpr explicit extents(_OtherIndexTypes... __dynvals) noexcept : m_vals(static_cast<index_type>(__dynvals)...) {}

  template <class _OtherIndexType, size_t _Num>
    requires(is_convertible_v<_OtherIndexType, index_type>&& is_nothrow_constructible_v<index_type, _OtherIndexType> &&
             (_Num == m_rank || _Num == m_rank_dynamic))
  explicit(_Num != m_rank_dynamic) constexpr extents(const array<_OtherIndexType, _Num>& exts) noexcept
      : m_vals(std::move(exts)) {}

  template <class _OtherIndexType, size_t _Num>
    requires(is_convertible_v<_OtherIndexType, index_type>&& is_nothrow_constructible_v<index_type, _OtherIndexType> &&
             (_Num == m_rank || _Num == m_rank_dynamic))
  explicit(_Num != m_rank_dynamic) constexpr extents(const span<_OtherIndexType, _Num>& exts) noexcept
      : m_vals(std::move(exts)) {}

private:
  // Function to construct extents storage from other extents.
  // With C++ 17 the first two variants could be collapsed using if constexpr
  // in which case you don't need all the requires clauses.
  // in C++ 14 mode that doesn't work due to infinite recursion
  template <size_t _DynCount, size_t _R, class _OtherExtents, class... _DynamicValues>
    requires((_R < m_rank) && (static_extent(_R) == dynamic_extent))
  vals_t __construct_vals_from_extents(
      std::integral_constant<size_t, _DynCount>,
      std::integral_constant<size_t, _R>,
      const _OtherExtents& exts,
      _DynamicValues... dynamic_values)
  noexcept {
    return __construct_vals_from_extents(
        std::integral_constant<size_t, _DynCount + 1>(),
        std::integral_constant<size_t, _R + 1>(),
        exts,
        dynamic_values...,
        exts.extent(_R));
  }

  template <size_t _DynCount, size_t _R, class _OtherExtents, class... _DynamicValues>
    requires((_R < m_rank) && (static_extent(_R) != dynamic_extent))
  vals_t __construct_vals_from_extents(
      std::integral_constant<size_t, _DynCount>,
      std::integral_constant<size_t, _R>,
      const _OtherExtents& exts,
      _DynamicValues... dynamic_values)
  noexcept {
    return __construct_vals_from_extents(
        std::integral_constant<size_t, _DynCount>(), std::integral_constant<size_t, _R + 1>(), exts, dynamic_values...);
  }

  template <size_t _DynCount, size_t _R, class _OtherExtents, class... _DynamicValues>
    requires((_R == m_rank) && (_DynCount == m_rank_dynamic))
  vals_t __construct_vals_from_extents(
      std::integral_constant<size_t, _DynCount>,
      std::integral_constant<size_t, _R>,
      const _OtherExtents&,
      _DynamicValues... dynamic_values)
  noexcept { return vals_t{static_cast<index_type>(dynamic_values)...}; }

public:
  // Converting constructor from other extents specializations
  template <class _OtherIndexType, size_t... _OtherExtents>
    requires((sizeof...(_OtherExtents) == sizeof...(_Extents)) &&
             ((_OtherExtents == dynamic_extent || _Extents == dynamic_extent || _OtherExtents == _Extents) && ...))
  explicit(
      (((_Extents != dynamic_extent) && (_OtherExtents == dynamic_extent)) || ...) ||
      (std::numeric_limits<index_type>::max() <
       std::numeric_limits<_OtherIndexType>::max())) constexpr extents(const extents<_OtherIndexType, _OtherExtents...>&
                                                                          other) noexcept
      : m_vals(__construct_vals_from_extents(
            std::integral_constant<size_t, 0>(), std::integral_constant<size_t, 0>(), other)) {}

  // Comparison operator
  template <class _OtherIndexType, size_t... _OtherExtents>
  friend constexpr bool operator==(const extents& lhs, const extents<_OtherIndexType, _OtherExtents...>& rhs) noexcept {
    bool value = true;
    if constexpr (rank() != sizeof...(_OtherExtents)) {
      value = false;
    } else {
      for (size_type r = 0; r < m_rank; r++)
        value &= static_cast<index_type>(rhs.extent(r)) == lhs.extent(r);
    }
    return value;
  }

  template <class _OtherIndexType, size_t... _OtherExtents>
  friend constexpr bool operator!=(extents const& lhs, extents<_OtherIndexType, _OtherExtents...> const& rhs) noexcept {
    return !(lhs == rhs);
  }
};

// _Recursive helper classes to implement dextents alias for extents
namespace __mdspan_detail {

template <class _IndexType, size_t _Rank, class _Extents = extents<_IndexType>>
struct __make_dextents;

template <class _IndexType, size_t _Rank, size_t... _ExtentsPack>
struct __make_dextents< _IndexType, _Rank, extents<_IndexType, _ExtentsPack...>> {
  using type = typename __make_dextents< _IndexType, _Rank - 1, extents<_IndexType, dynamic_extent, _ExtentsPack...>>::type;
};

template <class _IndexType, size_t... _ExtentsPack>
struct __make_dextents< _IndexType, 0, extents<_IndexType, _ExtentsPack...>> {
  using type = extents<_IndexType, _ExtentsPack...>;
};

} // end namespace __mdspan_detail

// [mdspan.extents.dextents], alias template
template <class _IndexType, size_t _Rank>
using dextents = typename __mdspan_detail::__make_dextents<_IndexType, _Rank>::type;

// Deduction guide for extents
template <class... _IndexTypes>
extents(_IndexTypes...) -> extents<size_t, size_t((_IndexTypes(), dynamic_extent))...>;

// Helper type traits for identifying a class as extents.
namespace __mdspan_detail {

template <class _T>
struct __is_extents : ::std::false_type {};

template <class _IndexType, size_t... _ExtentsPack>
struct __is_extents<extents<_IndexType, _ExtentsPack...>> : ::std::true_type {};

template <class _T>
inline constexpr bool __is_extents_v = __is_extents<_T>::value;

} // namespace __mdspan_detail

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___MDSPAN_EXTENTS_H
