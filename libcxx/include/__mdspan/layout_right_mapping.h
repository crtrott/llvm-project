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

#ifndef _LIBCPP___MDSPAN_LAYOUT_RIGHT_MAPPING_H
#define _LIBCPP___MDSPAN_LAYOUT_RIGHT_MAPPING_H

#include <__config>
#include <__mdspan/extents.h>
#include <__mdspan/layouts.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

template <class _Extents>
class layout_right::mapping {
  static_assert(__mdspan_detail::__is_extents<_Extents>::value,
                "layout_right::mapping template argument must be a specialization of extents");

  // For dynamic_rank()==0 check representability

public:
  using extents_type = _Extents;
  using index_type   = typename extents_type::index_type;
  using size_type    = typename extents_type::size_type;
  using rank_type    = typename extents_type::rank_type;
  using layout_type  = layout_right;

  // [mdspan.layout.right.cons], constructors
  constexpr mapping() noexcept               = default;
  constexpr mapping(const mapping&) noexcept = default;
  constexpr mapping(const extents_type& __ext) noexcept : __extents_(__ext) {}

  template <class _OtherExtents>
  constexpr explicit(!is_convertible_v<_OtherExtents, extents_type>) mapping(const mapping<_OtherExtents>&) noexcept {}

// add when we add other layouts
#  if 0
    template<class _OtherExtents>
      constexpr explicit(!is_convertible_v<_OtherExtents, extents_type>)
        mapping(const layout_left::mapping<_OtherExtents>&) noexcept {}

    template<class _OtherExtents>
      constexpr explicit(extents_type::rank() > 0)
        mapping(const layout_stride::mapping_<OtherExtents>&) noexcept;
#  endif

  constexpr mapping& operator=(const mapping&) noexcept = default;

  // [mdspan.layout.right.obs], observers
  constexpr const extents_type& extents() const noexcept { return __extents_; }

  constexpr index_type required_span_size() const noexcept {
    index_type __size = 1;
    for (size_t __r = 0; __r < extents_type::rank(); __r++)
      __size *= __extents_.extent(__r);
    return __size;
  }
  template <class... _Indices>
  constexpr index_type operator()(_Indices...) const noexcept {
    return 0;
  }

  static constexpr bool is_always_unique() noexcept { return true; }
  static constexpr bool is_always_exhaustive() noexcept { return true; }
  static constexpr bool is_always_strided() noexcept { return true; }

  static constexpr bool is_unique() noexcept { return true; }
  static constexpr bool is_exhaustive() noexcept { return true; }
  static constexpr bool is_strided() noexcept { return true; }

  constexpr index_type stride(rank_type) const noexcept { return 0; }

  template <class _OtherExtents>
  friend constexpr bool operator==(const mapping&, const mapping<_OtherExtents>&) noexcept {
    return false;
  }

private:
  extents_type __extents_{}; // exposition only
};

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___MDSPAN_LAYOUT_RIGHT_MAPPING_H
