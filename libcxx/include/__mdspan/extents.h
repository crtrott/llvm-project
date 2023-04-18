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

#include <__assert>
#include <__config>
#include <__fwd/array.h>
#include <__type_traits/is_convertible.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/make_unsigned.h>
#include <__utility/move.h>
#include <cinttypes>
#include <cstddef>
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
template <size_t _Idx, class _Tp, _Tp... _Extents>
struct __static_array_impl;

template <size_t _Idx, class _Tp, _Tp _FirstExt, _Tp... _Extents>
struct __static_array_impl<_Idx, _Tp, _FirstExt, _Extents...> {
  constexpr static _Tp get(size_t __r) noexcept {
    if (__r == _Idx)
      return _FirstExt;
    else
      return __static_array_impl<_Idx + 1, _Tp, _Extents...>::get(__r);
  }
  template <size_t __r>
  _LIBCPP_HIDE_FROM_ABI constexpr static _Tp get() {
    if constexpr (__r == _Idx)
      return _FirstExt;
    else
      return __static_array_impl<_Idx + 1, _Tp, _Extents...>::template get<__r>();
  }
};

// End the recursion
template <size_t _Idx, class _Tp, _Tp _FirstExt>
struct __static_array_impl<_Idx, _Tp, _FirstExt> {
  constexpr static _Tp get(size_t) noexcept { return _FirstExt; }
  template <size_t>
  _LIBCPP_HIDE_FROM_ABI constexpr static _Tp get() {
    return _FirstExt;
  }
};

// Don't start recursion if size 0
template <class _Tp>
struct __static_array_impl<0, _Tp> {
  constexpr static _Tp get(size_t) noexcept { return _Tp(); }
  template <size_t>
  _LIBCPP_HIDE_FROM_ABI constexpr static _Tp get() {
    return _Tp();
  }
};

// Static array, provides get<r>(), get(r) and operator[r]
template <class _Tp, _Tp... _Values>
struct __static_array : public __static_array_impl<0, _Tp, _Values...> {
public:
  _LIBCPP_HIDE_FROM_ABI constexpr static size_t size() { return sizeof...(_Values); }
};

// ------------------------------------------------------------------
// ------------ index_sequence_scan ---------------------------------
// ------------------------------------------------------------------

// index_sequence_scan takes compile time values and provides get(r)
//  which return the sum of the first r-1 values.

// Recursive implementation for get
template <size_t _Idx, size_t... _Values>
struct __index_sequence_scan_impl;

template <size_t _Idx, size_t _FirstVal, size_t... _Values>
struct __index_sequence_scan_impl<_Idx, _FirstVal, _Values...> {
  constexpr static size_t get(size_t __r) {
    if (__r > _Idx)
      return _FirstVal + __index_sequence_scan_impl<_Idx + 1, _Values...>::get(__r);
    else
      return 0;
  }
};

template <size_t _Idx, size_t _FirstVal>
struct __index_sequence_scan_impl<_Idx, _FirstVal> {
#  if defined(__NVCC__) || defined(__NVCOMPILER)
  // NVCC warns about pointless comparison with 0 for _Idx==0 and r being const
  // evaluatable and also 0.
  constexpr static size_t get(size_t __r) {
    return static_cast<int64_t>(_Idx) > static_cast<int64_t>(__r) ? _FirstVal : 0;
  }
#  else
  constexpr static size_t get(size_t __r) { return _Idx > __r ? _FirstVal : 0; }
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

template <class _Tp, size_t _Num>
struct __possibly_empty_array {
  _Tp __vals_[_Num];
  _LIBCPP_HIDE_FROM_ABI constexpr _Tp& operator[](size_t __r) { return __vals_[__r]; }
  _LIBCPP_HIDE_FROM_ABI constexpr const _Tp& operator[](size_t __r) const { return __vals_[__r]; }
};

template <class _Tp>
struct __possibly_empty_array<_Tp, 0> {
  _LIBCPP_HIDE_FROM_ABI constexpr _Tp operator[](size_t) { return _Tp(); }
  _LIBCPP_HIDE_FROM_ABI constexpr const _Tp operator[](size_t) const { return _Tp(); }
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
  using __static_vals_t                   = __static_array<_TStatic, _Values...>;
  constexpr static size_t __size_         = sizeof...(_Values);
  constexpr static size_t __size_dynamic_ = ((_Values == _DynTag) + ... + 0);

  // Dynamic values member
  [[no_unique_address]] __possibly_empty_array<_TDynamic, __size_dynamic_> __dyn_vals_;

  // static mapping of indices to the position in the dynamic values array
  using __dyn_map_t = __index_sequence_scan_impl<0, static_cast<size_t>(_Values == _DynTag)...>;

public:
  _LIBCPP_HIDE_FROM_ABI constexpr __maybe_static_array() = default;

  // constructor for all static values
  // TODO: add precondition check?
  template <class... _Vals>
    requires((__size_dynamic_ == 0) && (sizeof...(_Vals) > 0))
  _LIBCPP_HIDE_FROM_ABI constexpr __maybe_static_array(_Vals...) : __dyn_vals_{} {}

  // constructors from dynamic values only
  template <class... _DynVals>
    requires(sizeof...(_DynVals) == __size_dynamic_ && __size_dynamic_ > 0)
  _LIBCPP_HIDE_FROM_ABI constexpr __maybe_static_array(_DynVals... __vals)
      : __dyn_vals_{static_cast<_TDynamic>(__vals)...} {}

  template <class _Tp, size_t _Num>
    requires(_Num == __size_dynamic_ && _Num > 0)
  _LIBCPP_HIDE_FROM_ABI constexpr __maybe_static_array(const std::array<_Tp, _Num>& __vals) {
    for (size_t __r = 0; __r < _Num; __r++)
      __dyn_vals_[__r] = static_cast<_TDynamic>(__vals[__r]);
  }

  template <class _Tp, size_t _Num>
    requires(_Num == __size_dynamic_ && _Num == 0)
  _LIBCPP_HIDE_FROM_ABI constexpr __maybe_static_array(const std::array<_Tp, _Num>&) : __dyn_vals_{} {}

  template <class _Tp, size_t _Num >
    requires(_Num == __size_dynamic_ && _Num > 0)
  _LIBCPP_HIDE_FROM_ABI constexpr __maybe_static_array(const std::span<_Tp, _Num>& __vals) {
    for (size_t __r = 0; __r < _Num; __r++)
      __dyn_vals_[__r] = static_cast<_TDynamic>(__vals[__r]);
  }

  template <class _Tp, size_t _Num>
    requires(_Num == __size_dynamic_ && _Num == 0)
  _LIBCPP_HIDE_FROM_ABI constexpr __maybe_static_array(const std::span<_Tp, _Num>&) : __dyn_vals_{} {}

  // constructors from all values
  template <class... _DynVals>
    requires(sizeof...(_DynVals) != __size_dynamic_ && __size_dynamic_ > 0)
  _LIBCPP_HIDE_FROM_ABI constexpr __maybe_static_array(_DynVals... __vals) {
    static_assert((sizeof...(_DynVals) == __size_), "Invalid number of values.");
    _TDynamic __values[__size_]{static_cast<_TDynamic>(__vals)...};
    for (size_t __r = 0; __r < __size_; __r++) {
      _TStatic __static_val = __static_vals_t::get(__r);
      if (__static_val == _DynTag) {
        __dyn_vals_[__dyn_map_t::get(__r)] = __values[__r];
      }
      // Precondition check
      else
        _LIBCPP_ASSERT(__values[__r] == static_cast<_TDynamic>(__static_val),
                       "extents construction: mismatch of provided arguments with static extents.");
    }
  }

  template <class _Tp, size_t _Num>
    requires(_Num != __size_dynamic_ && __size_dynamic_ > 0)
  _LIBCPP_HIDE_FROM_ABI constexpr __maybe_static_array(const std::array<_Tp, _Num>& __vals) {
    static_assert((_Num == __size_), "Invalid number of values.");
    for (size_t __r = 0; __r < __size_; __r++) {
      _TStatic __static_val = __static_vals_t::get(__r);
      if (__static_val == _DynTag) {
        __dyn_vals_[__dyn_map_t::get(__r)] = static_cast<_TDynamic>(__vals[__r]);
      }
      // Precondition check
      else
        _LIBCPP_ASSERT(static_cast<_TDynamic>(__vals[__r]) == static_cast<_TDynamic>(__static_val),
                       "extents construction: mismatch of provided arguments with static extents.");
    }
  }

  template <class _Tp, size_t _Num>
    requires(_Num != __size_dynamic_ && __size_dynamic_ > 0)
  _LIBCPP_HIDE_FROM_ABI constexpr __maybe_static_array(const std::span<_Tp, _Num>& __vals) {
    static_assert((_Num == __size_) || (__size_ == dynamic_extent));
    for (size_t __r = 0; __r < __size_; __r++) {
      _TStatic __static_val = __static_vals_t::get(__r);
      if (__static_val == _DynTag) {
        __dyn_vals_[__dyn_map_t::get(__r)] = static_cast<_TDynamic>(__vals[__r]);
      }
      // Precondition check
      else
        _LIBCPP_ASSERT(static_cast<_TDynamic>(__vals[__r]) == static_cast<_TDynamic>(__static_val),
                       "extents construction: mismatch of provided arguments with static extents.");
    }
  }

  // access functions
  _LIBCPP_HIDE_FROM_ABI constexpr static _TStatic __static_value(size_t __r) noexcept {
    return __static_vals_t::get(__r);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr _TDynamic __value(size_t __r) const {
    _TStatic __static_val = __static_vals_t::get(__r);
    return __static_val == _DynTag ? __dyn_vals_[__dyn_map_t::get(__r)] : static_cast<_TDynamic>(__static_val);
  }
  _LIBCPP_HIDE_FROM_ABI constexpr _TDynamic operator[](size_t __r) const { return __value(__r); }

  // observers
  _LIBCPP_HIDE_FROM_ABI constexpr static size_t __size() { return __size_; }
  _LIBCPP_HIDE_FROM_ABI constexpr static size_t __size_dynamic() { return __size_dynamic_; }
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
  constexpr static rank_type __rank_         = sizeof...(_Extents);
  constexpr static rank_type __rank_dynamic_ = ((_Extents == dynamic_extent) + ... + 0);

  // internal storage type using __maybe_static_array
  using __vals_t = __mdspan_detail::__maybe_static_array<_IndexType, size_t, dynamic_extent, _Extents...>;
  [[no_unique_address]] __vals_t __vals_;

public:
  // [mdspan.extents.obs], observers of multidimensional index space
  _LIBCPP_HIDE_FROM_ABI constexpr static rank_type rank() noexcept { return __rank_; }
  _LIBCPP_HIDE_FROM_ABI constexpr static rank_type rank_dynamic() noexcept { return __rank_dynamic_; }

  _LIBCPP_HIDE_FROM_ABI constexpr index_type extent(rank_type __r) const noexcept { return __vals_.__value(__r); }
  _LIBCPP_HIDE_FROM_ABI constexpr static size_t static_extent(rank_type __r) noexcept {
    return __vals_t::__static_value(__r);
  }

  // [mdspan.extents.cons], constructors
  _LIBCPP_HIDE_FROM_ABI constexpr extents() noexcept = default;

  // Construction from just dynamic or all values.
  // Precondition check is deferred to __maybe_static_array constructor
  template <class... _OtherIndexTypes>
    requires((is_convertible_v<_OtherIndexTypes, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, _OtherIndexTypes> && ...) &&
             (sizeof...(_OtherIndexTypes) == __rank_ || sizeof...(_OtherIndexTypes) == __rank_dynamic_))
  _LIBCPP_HIDE_FROM_ABI constexpr explicit extents(_OtherIndexTypes... __dynvals) noexcept
      : __vals_(static_cast<index_type>(__dynvals)...) {}

  template <class _OtherIndexType, size_t _Num>
    requires(is_convertible_v<_OtherIndexType, index_type> && is_nothrow_constructible_v<index_type, _OtherIndexType> &&
             (_Num == __rank_ || _Num == __rank_dynamic_))
  explicit(_Num != __rank_dynamic_)
      _LIBCPP_HIDE_FROM_ABI constexpr extents(const array<_OtherIndexType, _Num>& __exts) noexcept
      : __vals_(std::move(__exts)) {}

  template <class _OtherIndexType, size_t _Num>
    requires(is_convertible_v<_OtherIndexType, index_type> && is_nothrow_constructible_v<index_type, _OtherIndexType> &&
             (_Num == __rank_ || _Num == __rank_dynamic_))
  explicit(_Num != __rank_dynamic_)
      _LIBCPP_HIDE_FROM_ABI constexpr extents(const span<_OtherIndexType, _Num>& __exts) noexcept
      : __vals_(std::move(__exts)) {}

private:
  // Function to construct extents storage from other extents.
  // With C++ 17 the first two variants could be collapsed using if constexpr
  // in which case you don't need all the requires clauses.
  // in C++ 14 mode that doesn't work due to infinite recursion
  template <size_t _DynCount, size_t _Idx, class _OtherExtents, class... _DynamicValues>
    requires((_Idx < __rank_) && (static_extent(_Idx) == dynamic_extent))
  _LIBCPP_HIDE_FROM_ABI __vals_t __construct_vals_from_extents(
      std::integral_constant<size_t, _DynCount>,
      std::integral_constant<size_t, _Idx>,
      const _OtherExtents& __exts,
      _DynamicValues... __dynamic_values) noexcept {
    return __construct_vals_from_extents(
        std::integral_constant<size_t, _DynCount + 1>(),
        std::integral_constant<size_t, _Idx + 1>(),
        __exts,
        __dynamic_values...,
        __exts.extent(_Idx));
  }

  template <size_t _DynCount, size_t _Idx, class _OtherExtents, class... _DynamicValues>
    requires((_Idx < __rank_) && (static_extent(_Idx) != dynamic_extent))
  _LIBCPP_HIDE_FROM_ABI __vals_t __construct_vals_from_extents(
      std::integral_constant<size_t, _DynCount>,
      std::integral_constant<size_t, _Idx>,
      const _OtherExtents& __exts,
      _DynamicValues... __dynamic_values) noexcept {
    return __construct_vals_from_extents(
        std::integral_constant<size_t, _DynCount>(),
        std::integral_constant<size_t, _Idx + 1>(),
        __exts,
        __dynamic_values...);
  }

  template <size_t _DynCount, size_t _Idx, class _OtherExtents, class... _DynamicValues>
    requires((_Idx == __rank_) && (_DynCount == __rank_dynamic_))
  _LIBCPP_HIDE_FROM_ABI __vals_t __construct_vals_from_extents(
      std::integral_constant<size_t, _DynCount>,
      std::integral_constant<size_t, _Idx>,
      const _OtherExtents&,
      _DynamicValues... __dynamic_values) noexcept {
    return __vals_t{static_cast<index_type>(__dynamic_values)...};
  }

public:
  // Converting constructor from other extents specializations
  template <class _OtherIndexType, size_t... _OtherExtents>
    requires((sizeof...(_OtherExtents) == sizeof...(_Extents)) &&
             ((_OtherExtents == dynamic_extent || _Extents == dynamic_extent || _OtherExtents == _Extents) && ...))
  explicit((((_Extents != dynamic_extent) && (_OtherExtents == dynamic_extent)) || ...) ||
           (static_cast<make_unsigned_t<index_type>>(numeric_limits<index_type>::max()) <
            static_cast<make_unsigned_t<_OtherIndexType>>(numeric_limits<_OtherIndexType>::max())))
      _LIBCPP_HIDE_FROM_ABI constexpr extents(const extents<_OtherIndexType, _OtherExtents...>& __other) noexcept
      : __vals_(__construct_vals_from_extents(
            std::integral_constant<size_t, 0>(), std::integral_constant<size_t, 0>(), __other)) {}

  // Comparison operator
  template <class _OtherIndexType, size_t... _OtherExtents>
  _LIBCPP_HIDE_FROM_ABI friend constexpr bool
  operator==(const extents& __lhs, const extents<_OtherIndexType, _OtherExtents...>& __rhs) noexcept {
    bool __value = true;
    if constexpr (rank() != sizeof...(_OtherExtents)) {
      __value = false;
    } else {
      for (rank_type __r = 0; __r < __rank_; __r++)
        __value &= static_cast<index_type>(__rhs.extent(__r)) == __lhs.extent(__r);
    }
    return __value;
  }

  template <class _OtherIndexType, size_t... _OtherExtents>
  _LIBCPP_HIDE_FROM_ABI friend constexpr bool
  operator!=(extents const& __lhs, extents<_OtherIndexType, _OtherExtents...> const& __rhs) noexcept {
    return !(__lhs == __rhs);
  }
};

// Recursive helper classes to implement dextents alias for extents
namespace __mdspan_detail {

template <class _IndexType, size_t _Rank, class _Extents = extents<_IndexType>>
struct __make_dextents;

template <class _IndexType, size_t _Rank, size_t... _ExtentsPack>
struct __make_dextents< _IndexType, _Rank, extents<_IndexType, _ExtentsPack...>> {
  using type =
      typename __make_dextents< _IndexType, _Rank - 1, extents<_IndexType, dynamic_extent, _ExtentsPack...>>::type;
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

template <class _Tp>
struct __is_extents : ::std::false_type {};

template <class _IndexType, size_t... _ExtentsPack>
struct __is_extents<extents<_IndexType, _ExtentsPack...>> : ::std::true_type {};

template <class _Tp>
inline constexpr bool __is_extents_v = __is_extents<_Tp>::value;

} // namespace __mdspan_detail

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___MDSPAN_EXTENTS_H
