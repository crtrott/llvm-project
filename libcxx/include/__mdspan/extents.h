// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===---------------------------------------------------------------------===//

#ifndef _LIBCPP_EXTENTS
#define _LIBCPP_EXTENTS

#include <array>
#include <span>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER > 20

// Implementation of Static Array, recursive implementation of get
template <size_t R, class T, T... Extents>
struct static_array_impl;

template <size_t R, class T, T FirstExt, T... Extents>
struct static_array_impl<R, T, FirstExt, Extents...> {
  constexpr static T value(size_t r) {
    if (r == R)
      return FirstExt;
    else
      return static_array_impl<R + 1, T, Extents...>::value(r);
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
  constexpr static T value(int) { return FirstExt; }
  template <size_t>
  constexpr static T get() {
    return FirstExt;
  }
};

// Don't start recursion if size 0
template <class T>
struct static_array_impl<0, T> {
  constexpr static T value(int) { return T(); }
  template <size_t>
  constexpr static T get() {
    return T();
  }
};

// Static array, provides get<r>(), get(r) and operator[r]
template <class T, T... Values>
struct static_array {
private:
  using impl_t = static_array_impl<0, T, Values...>;

public:
  using value_type = T;

  constexpr T operator[](int r) const { return get(r); }
  constexpr static T get(int r) { return impl_t::value(r); }
  template <size_t r>
  constexpr static T get() {
    return impl_t::template get<r>();
  }
  constexpr static size_t size() { return sizeof...(Values); }
};

// index_sequence_scan takes indicies and provides get(r) and get<r>() to get the sum of the first r-1 values
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
  template <size_t r>
  constexpr static size_t get() {
    return r > R ? FirstVal + index_sequence_scan_impl<R + 1, Values...>::get(r) : 0;
  }
};

template <size_t R, size_t FirstVal>
struct index_sequence_scan_impl<R, FirstVal> {
  constexpr static size_t get(size_t r) { return R > r ? FirstVal : 0; }
  template <size_t r>
  constexpr static size_t get() {
    return R > r ? FirstVal : 0;
  }
};
template <>
struct index_sequence_scan_impl<0> {
  constexpr static size_t get(size_t) { return 0; }
  template <size_t>
  constexpr static size_t get() {
    return 0;
  }
};

// Need this to have fully empty class maybe_static_array with all entries static
// Rather specialize this small helper class on size 0, then the whole thing
template <class T, size_t N>
struct possibly_empty_array {
  T vals[N];
  constexpr T& operator[](int r) { return vals[r]; }
  constexpr const T& operator[](int r) const { return vals[r]; }
};

template <class T>
struct possibly_empty_array<T, 0> {
  constexpr T operator[](int) { return T(); }
  constexpr const T operator[](int) const { return T(); }
};

// maybe_static_array is an array which has a mix of static and dynamic values
template <class TDynamic, class TStatic, TStatic dyn_tag, TStatic... Values>
struct maybe_static_array {
private:
  using static_vals_t                    = static_array<TStatic, Values...>;
  constexpr static size_t m_size         = sizeof...(Values);
  constexpr static size_t m_size_dynamic = ((Values == dyn_tag) + ... + 0);

  [[no_unique_address]] possibly_empty_array<TDynamic, m_size_dynamic> m_dyn_vals;

public:
  using dyn_map_t = index_sequence_scan_impl<0, size_t(Values == dyn_tag)...>;

  using value_type                             = TDynamic;
  using static_value_type                      = TStatic;
  constexpr static static_value_type tag_value = dyn_tag;

  constexpr maybe_static_array() = default;

  // constructors from dynamic_extentamic values only
  template <class... DynVals>
    requires(sizeof...(DynVals) == m_size_dynamic && m_size_dynamic > 0)
  constexpr maybe_static_array(DynVals... vals) : m_dyn_vals{static_cast<TDynamic>(vals)...} {}

  template <class... DynVals>
    requires(m_size_dynamic == 0)
  constexpr maybe_static_array(DynVals...) : m_dyn_vals{} {}

  template <class T, size_t N>
    requires(N == m_size_dynamic && N > 0)
  constexpr maybe_static_array(const std::array<T, N>& vals) {
    for (size_t r = 0; r < N; r++)
      m_dyn_vals[r] = static_cast<TDynamic>(vals[r]);
  }
  template <class T, size_t N>
    requires(N == m_size_dynamic && N == 0)
  constexpr maybe_static_array(const std::array<T, N>&) {}

  template <class T, size_t N>
    requires(N == m_size_dynamic)
  constexpr maybe_static_array(const std::span<T, N>& vals) {
    for (size_t r = 0; r < N; r++)
      m_dyn_vals[r] = static_cast<TDynamic>(vals[r]);
  }

  // constructors from all values
  template <class... DynVals>
    requires(sizeof...(DynVals) != m_size_dynamic && m_size_dynamic > 0)
  constexpr maybe_static_array(DynVals... vals) {
    TDynamic values[m_size]{static_cast<TDynamic>(vals)...};
    for (int r = 0; r < m_size; r++) {
      TStatic static_val = static_vals_t::get(r);
      if (static_val == dynamic_extent) {
        m_dyn_vals[dyn_map_t::get(r)] = values[r];
      }
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
    for (size_t r = 0; r < m_size; r++) {
      TStatic static_val = static_vals_t::get(r);
      if (static_val == dynamic_extent) {
        m_dyn_vals[dyn_map_t::get(r)] = static_cast<TDynamic>(vals[r]);
      }
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
    for (int r = 0; r < m_size; r++) {
      TStatic static_val = static_vals_t::get(r);
      if (static_val == dynamic_extent) {
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
  constexpr static TStatic static_value(int r) { return static_vals_t::get(r); }

  constexpr TDynamic value(int r) const {
    TStatic static_val = static_vals_t::get(r);
    return static_val == dynamic_extent ? m_dyn_vals[dyn_map_t::get(r)] : static_cast<TDynamic>(static_val);
  }
  constexpr TDynamic operator[](int r) const { return value(r); }

  // observers
  constexpr static size_t size() { return m_size; }
  constexpr static size_t size_dynamic() { return m_size_dynamic; }
};

#  if 0
template<class TDynamic, class TStatic, TStatic dyn_tag>
struct maybe_static_array {
  private:
    using static_vals_t = static_array<TStatic, Values...>;
    using dyn_map_t = index_sequence_scan_impl<0, size_t(Values==dyn_tag)...>;
    constexpr static size_t m_size = sizeof...(Values);
    constexpr static size_t m_size_dynamic = ((Values==dyn_tag) + ... + 0);


  public:

    using value_type = TDynamic;
    using static_value_type = TStatic;
    constexpr static static_value_type tag_value = dyn_tag;

    constexpr static size_t size() { return 0; }
    constexpr static size_t size_dynamic() { return 0; }
};
#  endif

template <class IndexType, size_t... Extents>
class extents {
public:
  using rank_type  = size_t;
  using index_type = IndexType;
  using size_type  = make_unsigned_t<index_type>;

private:
  constexpr static rank_type m_rank         = sizeof...(Extents);
  constexpr static rank_type m_rank_dynamic = ((Extents == dynamic_extent) + ... + 0);

  using vals_t = maybe_static_array<IndexType, size_t, dynamic_extent, Extents...>;
  [[no_unique_address]] vals_t m_vals;

public:
  constexpr static rank_type rank() noexcept { return m_rank; }
  constexpr static rank_type rank_dynamic() noexcept { return m_rank_dynamic; }

  constexpr extents() = default;

  template <class... OtherIndexTypes>
    requires((is_convertible_v<OtherIndexTypes, index_type> && ...) &&
             (is_nothrow_constructible_v<index_type, OtherIndexTypes> && ...) &&
             (sizeof...(OtherIndexTypes) == m_rank || sizeof...(OtherIndexTypes) == m_rank_dynamic))
  constexpr extents(OtherIndexTypes... dynvals) : m_vals(static_cast<index_type>(dynvals)...) {}

  template <class OtherIndexType, size_t N>
    requires(is_convertible_v<OtherIndexType, index_type>&& is_nothrow_constructible_v<index_type, OtherIndexType> &&
             (N == m_rank || N == m_rank_dynamic))
  constexpr extents(const array<OtherIndexType, N>& exts) : m_vals(std::move(exts)) {}

  template <class OtherIndexType, size_t N>
    requires(is_convertible_v<OtherIndexType, index_type>&& is_nothrow_constructible_v<index_type, OtherIndexType> &&
             (N == m_rank || N == m_rank_dynamic))
  constexpr extents(const span<OtherIndexType, N>& exts) : m_vals(std::move(exts)) {}

  template <class OtherIndexType, size_t... OtherExtents>
    requires((sizeof...(OtherExtents) == rank()) &&
             ((OtherExtents == dynamic_extent || Extents == dynamic_extent || OtherExtents == Extents) && ...))
  constexpr explicit((((Extents != dynamic_extent) && (OtherExtents == dynamic_extent)) || ...) ||
                     (numeric_limits<index_type>::max() < numeric_limits<OtherIndexType>::max()))
      extents(const extents<OtherIndexType, OtherExtents...>& other) noexcept {
    if constexpr (m_rank_dynamic > 0) {
      index_type vals[m_rank_dynamic];
      for (int r = 0; r < m_rank; r++) {
        if (static_extent(r) == dynamic_extent) {
          vals[vals_t::dyn_map_t::get(r)] = other.extent(r);
        }
      }
      m_vals = vals_t(span<index_type, m_rank_dynamic>(vals));
    }
  }

  constexpr index_type extent(int r) const { return m_vals.value(r); }
  constexpr static size_t static_extent(int r) { return vals_t::static_value(r); }

  template <class OtherIndexType, size_t... OtherExtents>
  friend constexpr bool operator==(const extents& ext, const extents<OtherIndexType, OtherExtents...>& ext2) noexcept {
    bool value = true;
    for (int r = 0; r < m_rank; r++)
      value &= ext.extent(r) == ext2.extent(r);
    return value;
  }
};

namespace detail {

template <class IndexType, size_t Rank, class Extents = ::std::extents<IndexType>>
struct __make_dextents;

template <class IndexType, size_t Rank, size_t... ExtentsPack>
struct __make_dextents<IndexType, Rank, ::std::extents<IndexType, ExtentsPack...>> {
  using type =
      typename __make_dextents< IndexType, Rank - 1, ::std::extents<IndexType, ::std::dynamic_extent, ExtentsPack...>>::
          type;
};

template <class IndexType, size_t... ExtentsPack>
struct __make_dextents<IndexType, 0, ::std::extents<IndexType, ExtentsPack...>> {
  using type = ::std::extents<IndexType, ExtentsPack...>;
};

} // end namespace detail

template <class IndexType, size_t Rank>
using dextents = typename detail::__make_dextents<IndexType, Rank>::type;

#  if defined(_MDSPAN_USE_CLASS_TEMPLATE_ARGUMENT_DEDUCTION)
template <class... IndexTypes>
extents(IndexTypes...) -> extents<size_t, size_t((IndexTypes(), ::std::dynamic_extent))...>;
#  endif

namespace detail {

template <class T>
struct __is_extents : ::std::false_type {};

template <class IndexType, size_t... ExtentsPack>
struct __is_extents<::std::extents<IndexType, ExtentsPack...>> : ::std::true_type {};

template <class T>
static constexpr bool __is_extents_v = __is_extents<T>::value;

} // namespace detail

#endif // _LIBCPP_STD_VER > 20

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_EXTENTS
