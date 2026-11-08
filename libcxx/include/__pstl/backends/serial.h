// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___PSTL_BACKENDS_SERIAL_H
#define _LIBCPP___PSTL_BACKENDS_SERIAL_H

#include <__algorithm/find_end.h>
#include <__algorithm/find_if.h>
#include <__algorithm/for_each.h>
#include <__algorithm/includes.h>
#include <__algorithm/is_heap_until.h>
#include <__algorithm/merge.h>
#include <__algorithm/min_element.h>
#include <__algorithm/minmax_element.h>
#include <__algorithm/mismatch.h>
#include <__algorithm/reverse.h>
#include <__algorithm/search.h>
#include <__algorithm/search_n.h>
#include <__algorithm/set_difference.h>
#include <__algorithm/set_intersection.h>
#include <__algorithm/set_symmetric_difference.h>
#include <__algorithm/set_union.h>
#include <__algorithm/stable_sort.h>
#include <__algorithm/swap_ranges.h>
#include <__algorithm/transform.h>
#include <__config>
#include <__memory/uninitialized_algorithms.h>
#include <__numeric/transform_inclusive_scan.h>
#include <__numeric/transform_reduce.h>
#include <__optional/optional.h>
#include <__pstl/backend_fwd.h>
#include <__utility/empty.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <__utility/pair.h>

#if defined(__AVM__)
#  include <__algorithm/adjacent_find.h>
#  include <__algorithm/all_of.h>
#  include <__algorithm/any_of.h>
#  include <__algorithm/copy.h>
#  include <__algorithm/copy_n.h>
#  include <__algorithm/count.h>
#  include <__algorithm/count_if.h>
#  include <__algorithm/equal.h>
#  include <__algorithm/fill.h>
#  include <__algorithm/fill_n.h>
#  include <__algorithm/find.h>
#  include <__algorithm/find_first_of.h>
#  include <__algorithm/find_if_not.h>
#  include <__algorithm/for_each_n.h>
#  include <__algorithm/generate.h>
#  include <__algorithm/generate_n.h>
#  include <__algorithm/is_heap.h>
#  include <__algorithm/is_partitioned.h>
#  include <__algorithm/is_sorted.h>
#  include <__algorithm/is_sorted_until.h>
#  include <__algorithm/lexicographical_compare.h>
#  include <__algorithm/max_element.h>
#  include <__algorithm/move.h>
#  include <__algorithm/none_of.h>
#  include <__algorithm/replace.h>
#  include <__algorithm/replace_copy.h>
#  include <__algorithm/replace_copy_if.h>
#  include <__algorithm/replace_if.h>
#  include <__algorithm/reverse_copy.h>
#  include <__algorithm/rotate_copy.h>
#  include <__algorithm/sort.h>
#  include <__memory/destroy.h>
#  include <__numeric/adjacent_difference.h>
#  include <__numeric/exclusive_scan.h>
#  include <__numeric/inclusive_scan.h>
#  include <__numeric/reduce.h>
#  include <__numeric/transform_exclusive_scan.h>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 17

_LIBCPP_BEGIN_NAMESPACE_STD
namespace __pstl {

#  if defined(__AVM__)
// Serial execution is valid for every standard policy. Dispatch directly to
// each serial algorithm so default-backend compositions do not retain nested
// callbacks and optional results on AVM's 256-byte stack. The noexcept boundary
// still terminates when an element-access function throws.
#    define _LIBCPP_AVM_PSTL_VALUE(_Backend, _Algorithm)                                                               \
      template <class _ExecutionPolicy>                                                                                \
      struct _Backend<__serial_backend_tag, _ExecutionPolicy> {                                                        \
        template <class _Policy, class... _Args>                                                                       \
        _LIBCPP_HIDE_FROM_ABI auto operator()(_Policy&&, _Args&&... __args) const noexcept                             \
            -> optional<decltype(std::_Algorithm(std::forward<_Args>(__args)...))> {                                   \
          return std::_Algorithm(std::forward<_Args>(__args)...);                                                      \
        }                                                                                                              \
      }
#    define _LIBCPP_AVM_PSTL_VOID(_Backend, _Algorithm)                                                                \
      template <class _ExecutionPolicy>                                                                                \
      struct _Backend<__serial_backend_tag, _ExecutionPolicy> {                                                        \
        template <class _Policy, class... _Args>                                                                       \
        _LIBCPP_HIDE_FROM_ABI optional<__empty> operator()(_Policy&&, _Args&&... __args) const noexcept {              \
          (void)std::_Algorithm(std::forward<_Args>(__args)...);                                                       \
          return __empty{};                                                                                            \
        }                                                                                                              \
      }

_LIBCPP_AVM_PSTL_VALUE(__adjacent_difference, adjacent_difference);
_LIBCPP_AVM_PSTL_VALUE(__adjacent_find, adjacent_find);
_LIBCPP_AVM_PSTL_VALUE(__all_of, all_of);
_LIBCPP_AVM_PSTL_VALUE(__any_of, any_of);
_LIBCPP_AVM_PSTL_VALUE(__copy, copy);
_LIBCPP_AVM_PSTL_VALUE(__copy_n, copy_n);
_LIBCPP_AVM_PSTL_VALUE(__count, count);
_LIBCPP_AVM_PSTL_VALUE(__count_if, count_if);
_LIBCPP_AVM_PSTL_VALUE(__destroy_n, destroy_n);
_LIBCPP_AVM_PSTL_VALUE(__equal, equal);
_LIBCPP_AVM_PSTL_VALUE(__exclusive_scan, exclusive_scan);
_LIBCPP_AVM_PSTL_VALUE(__fill_n, fill_n);
_LIBCPP_AVM_PSTL_VALUE(__find, find);
_LIBCPP_AVM_PSTL_VALUE(__find_end, find_end);
_LIBCPP_AVM_PSTL_VALUE(__find_first_of, find_first_of);
_LIBCPP_AVM_PSTL_VALUE(__find_if, find_if);
_LIBCPP_AVM_PSTL_VALUE(__find_if_not, find_if_not);
_LIBCPP_AVM_PSTL_VALUE(__for_each_n, for_each_n);
_LIBCPP_AVM_PSTL_VALUE(__generate_n, generate_n);
_LIBCPP_AVM_PSTL_VALUE(__includes, includes);
_LIBCPP_AVM_PSTL_VALUE(__inclusive_scan, inclusive_scan);
_LIBCPP_AVM_PSTL_VALUE(__is_heap, is_heap);
_LIBCPP_AVM_PSTL_VALUE(__is_heap_until, is_heap_until);
_LIBCPP_AVM_PSTL_VALUE(__is_partitioned, is_partitioned);
_LIBCPP_AVM_PSTL_VALUE(__is_sorted, is_sorted);
_LIBCPP_AVM_PSTL_VALUE(__is_sorted_until, is_sorted_until);
_LIBCPP_AVM_PSTL_VALUE(__lexicographical_compare, lexicographical_compare);
_LIBCPP_AVM_PSTL_VALUE(__max_element, max_element);
_LIBCPP_AVM_PSTL_VALUE(__merge, merge);
_LIBCPP_AVM_PSTL_VALUE(__min_element, min_element);
_LIBCPP_AVM_PSTL_VALUE(__minmax_element, minmax_element);
_LIBCPP_AVM_PSTL_VALUE(__mismatch, mismatch);
_LIBCPP_AVM_PSTL_VALUE(__move, move);
_LIBCPP_AVM_PSTL_VALUE(__none_of, none_of);
_LIBCPP_AVM_PSTL_VALUE(__reduce, reduce);
_LIBCPP_AVM_PSTL_VALUE(__reverse_copy, reverse_copy);
_LIBCPP_AVM_PSTL_VALUE(__rotate_copy, rotate_copy);
_LIBCPP_AVM_PSTL_VALUE(__search, search);
_LIBCPP_AVM_PSTL_VALUE(__search_n, search_n);
_LIBCPP_AVM_PSTL_VALUE(__set_difference, set_difference);
_LIBCPP_AVM_PSTL_VALUE(__set_intersection, set_intersection);
_LIBCPP_AVM_PSTL_VALUE(__set_symmetric_difference, set_symmetric_difference);
_LIBCPP_AVM_PSTL_VALUE(__set_union, set_union);
_LIBCPP_AVM_PSTL_VALUE(__swap_ranges, swap_ranges);
_LIBCPP_AVM_PSTL_VALUE(__transform, transform);
_LIBCPP_AVM_PSTL_VALUE(__transform_exclusive_scan, transform_exclusive_scan);
_LIBCPP_AVM_PSTL_VALUE(__transform_inclusive_scan, transform_inclusive_scan);
_LIBCPP_AVM_PSTL_VALUE(__transform_reduce, transform_reduce);
_LIBCPP_AVM_PSTL_VALUE(__uninitialized_copy, uninitialized_copy);
_LIBCPP_AVM_PSTL_VALUE(__uninitialized_copy_n, uninitialized_copy_n);
_LIBCPP_AVM_PSTL_VALUE(__uninitialized_default_construct_n, uninitialized_default_construct_n);
_LIBCPP_AVM_PSTL_VALUE(__uninitialized_fill_n, uninitialized_fill_n);
_LIBCPP_AVM_PSTL_VALUE(__uninitialized_move, uninitialized_move);
_LIBCPP_AVM_PSTL_VALUE(__uninitialized_move_n, uninitialized_move_n);
_LIBCPP_AVM_PSTL_VALUE(__uninitialized_value_construct_n, uninitialized_value_construct_n);
_LIBCPP_AVM_PSTL_VALUE(__mismatch_3leg, mismatch);
_LIBCPP_AVM_PSTL_VALUE(__equal_3leg, equal);
_LIBCPP_AVM_PSTL_VALUE(__transform_binary, transform);
_LIBCPP_AVM_PSTL_VALUE(__transform_reduce_binary, transform_reduce);
_LIBCPP_AVM_PSTL_VALUE(__inclusive_scan_init, inclusive_scan);
_LIBCPP_AVM_PSTL_VALUE(__transform_inclusive_scan_init, transform_inclusive_scan);

_LIBCPP_AVM_PSTL_VOID(__destroy, destroy);
_LIBCPP_AVM_PSTL_VOID(__fill, fill);
_LIBCPP_AVM_PSTL_VOID(__for_each, for_each);
_LIBCPP_AVM_PSTL_VOID(__generate, generate);
_LIBCPP_AVM_PSTL_VOID(__replace, replace);
_LIBCPP_AVM_PSTL_VOID(__replace_copy, replace_copy);
_LIBCPP_AVM_PSTL_VOID(__replace_copy_if, replace_copy_if);
_LIBCPP_AVM_PSTL_VOID(__replace_if, replace_if);
_LIBCPP_AVM_PSTL_VOID(__reverse, reverse);
_LIBCPP_AVM_PSTL_VOID(__sort, sort);
_LIBCPP_AVM_PSTL_VOID(__stable_sort, stable_sort);
_LIBCPP_AVM_PSTL_VOID(__uninitialized_default_construct, uninitialized_default_construct);
_LIBCPP_AVM_PSTL_VOID(__uninitialized_fill, uninitialized_fill);
_LIBCPP_AVM_PSTL_VOID(__uninitialized_value_construct, uninitialized_value_construct);

#    undef _LIBCPP_AVM_PSTL_VALUE
#    undef _LIBCPP_AVM_PSTL_VOID
#  else

//
// This partial PSTL backend runs everything serially.
//
// TODO: Right now, the serial backend must be used with another backend
//       like the "default backend" because it doesn't implement all the
//       necessary PSTL operations. It would be better to dispatch all
//       algorithms to their serial counterpart directly, since this can
//       often be more efficient than the "default backend"'s implementation
//       if we end up running serially anyways.
//

template <class _ExecutionPolicy>
struct __find_end<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
  _LIBCPP_HIDE_FROM_ABI optional<_ForwardIterator1>
  operator()(_Policy&&,
             _ForwardIterator1 __first1,
             _ForwardIterator1 __last1,
             _ForwardIterator2 __first2,
             _ForwardIterator2 __last2,
             _BinaryPredicate __pred) const noexcept {
    return std::find_end(
        std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2), std::move(__pred));
  }
};

template <class _ExecutionPolicy>
struct __find_if<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Pred>
  _LIBCPP_HIDE_FROM_ABI optional<_ForwardIterator>
  operator()(_Policy&&, _ForwardIterator __first, _ForwardIterator __last, _Pred&& __pred) const noexcept {
    return std::find_if(std::move(__first), std::move(__last), std::forward<_Pred>(__pred));
  }
};

template <class _ExecutionPolicy>
struct __mismatch<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator1, class _ForwardIterator2, class _Predicate>
  _LIBCPP_HIDE_FROM_ABI optional<pair<_ForwardIterator1, _ForwardIterator2>>
  operator()(_Policy&&,
             _ForwardIterator1 __first1,
             _ForwardIterator1 __last1,
             _ForwardIterator2 __first2,
             _ForwardIterator2 __last2,
             _Predicate&& __pred) const noexcept {
    return std::mismatch(
        std::move(__first1),
        std::move(__last1),
        std::move(__first2),
        std::move(__last2),
        std::forward<_Predicate>(__pred));
  }
};

template <class _ExecutionPolicy>
struct __for_each<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Function>
  _LIBCPP_HIDE_FROM_ABI optional<__empty>
  operator()(_Policy&&, _ForwardIterator __first, _ForwardIterator __last, _Function&& __func) const noexcept {
    std::for_each(std::move(__first), std::move(__last), std::forward<_Function>(__func));
    return __empty{};
  }
};

template <class _Backend, class _RawExecutionPolicy>
struct __is_heap_until {
  template <class _Policy, class _RandomAccessIterator, class _Comp>
  _LIBCPP_HIDE_FROM_ABI optional<_RandomAccessIterator>
  operator()(_Policy&&, _RandomAccessIterator __first, _RandomAccessIterator __last, _Comp __comp) const noexcept {
    return std::is_heap_until(std::move(__first), std::move(__last), std::move(__comp));
  }
};

template <class _ExecutionPolicy>
struct __merge<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardOutIterator, class _Comp>
  _LIBCPP_HIDE_FROM_ABI optional<_ForwardOutIterator> operator()(
      _Policy&&,
      _ForwardIterator1 __first1,
      _ForwardIterator1 __last1,
      _ForwardIterator2 __first2,
      _ForwardIterator2 __last2,
      _ForwardOutIterator __outit,
      _Comp&& __comp) const noexcept {
    return std::merge(
        std::move(__first1),
        std::move(__last1),
        std::move(__first2),
        std::move(__last2),
        std::move(__outit),
        std::forward<_Comp>(__comp));
  }
};

template <class _ExecutionPolicy>
struct __min_element<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Compare>
  _LIBCPP_HIDE_FROM_ABI optional<_ForwardIterator>
  operator()(_Policy&&, _ForwardIterator __first, _ForwardIterator __last, _Compare __comp) const noexcept {
    return std::min_element(std::move(__first), std::move(__last), std::move(__comp));
  }
};

template <class _ExecutionPolicy>
struct __minmax_element<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Compare>
  _LIBCPP_HIDE_FROM_ABI optional<pair<_ForwardIterator, _ForwardIterator>>
  operator()(_Policy&&, _ForwardIterator __first, _ForwardIterator __last, _Compare __comp) const noexcept {
    return std::minmax_element(std::move(__first), std::move(__last), std::move(__comp));
  }
};

template <class _ExecutionPolicy>
struct __reverse<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _BidirectionalIterator>
  _LIBCPP_HIDE_FROM_ABI optional<__empty>
  operator()(_Policy&&, _BidirectionalIterator __first, _BidirectionalIterator __last) const noexcept {
    std::reverse(std::move(__first), std::move(__last));
    return __empty{};
  }
};

template <class _ExecutionPolicy>
struct __search<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator1, class _ForwardIterator2, class _BinaryPredicate>
  _LIBCPP_HIDE_FROM_ABI optional<_ForwardIterator1>
  operator()(_Policy&&,
             _ForwardIterator1 __first1,
             _ForwardIterator1 __last1,
             _ForwardIterator2 __first2,
             _ForwardIterator2 __last2,
             _BinaryPredicate __pred) const noexcept {
    return std::search(
        std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2), std::move(__pred));
  }
};

template <class _ExecutionPolicy>
struct __search_n<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Size, class _Tp, class _Predicate>
  _LIBCPP_HIDE_FROM_ABI optional<_ForwardIterator>
  operator()(_Policy&&,
             _ForwardIterator __first,
             _ForwardIterator __last,
             _Size __count,
             const _Tp& __value,
             _Predicate __pred) const noexcept {
    return std::search_n(std::move(__first), std::move(__last), __count, __value, std::move(__pred));
  }
};

template <class _ExecutionPolicy>
struct __stable_sort<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _RandomAccessIterator, class _Comp>
  _LIBCPP_HIDE_FROM_ABI optional<__empty>
  operator()(_Policy&&, _RandomAccessIterator __first, _RandomAccessIterator __last, _Comp&& __comp) const noexcept {
    std::stable_sort(std::move(__first), std::move(__last), std::forward<_Comp>(__comp));
    return __empty{};
  }
};

template <class _ExecutionPolicy>
struct __swap_ranges<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator1, class _ForwardIterator2>
  _LIBCPP_HIDE_FROM_ABI optional<_ForwardIterator2> operator()(
      _Policy&&, _ForwardIterator1 __first1, _ForwardIterator1 __last1, _ForwardIterator2 __first2) const noexcept {
    return std::swap_ranges(std::move(__first1), std::move(__last1), std::move(__first2));
  }
};

template <class _ExecutionPolicy>
struct __transform<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _ForwardOutIterator, class _UnaryOperation>
  _LIBCPP_HIDE_FROM_ABI optional<_ForwardOutIterator> operator()(
      _Policy&&, _ForwardIterator __first, _ForwardIterator __last, _ForwardOutIterator __outit, _UnaryOperation&& __op)
      const noexcept {
    return std::transform(
        std::move(__first), std::move(__last), std::move(__outit), std::forward<_UnaryOperation>(__op));
  }
};

template <class _ExecutionPolicy>
struct __transform_binary<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy,
            class _ForwardIterator1,
            class _ForwardIterator2,
            class _ForwardOutIterator,
            class _BinaryOperation>
  _LIBCPP_HIDE_FROM_ABI optional<_ForwardOutIterator>
  operator()(_Policy&&,
             _ForwardIterator1 __first1,
             _ForwardIterator1 __last1,
             _ForwardIterator2 __first2,
             _ForwardOutIterator __outit,
             _BinaryOperation&& __op) const noexcept {
    return std::transform(
        std::move(__first1),
        std::move(__last1),
        std::move(__first2),
        std::move(__outit),
        std::forward<_BinaryOperation>(__op));
  }
};

template <class _ExecutionPolicy>
struct __transform_inclusive_scan_init<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy,
            class _ForwardIterator1,
            class _ForwardIterator2,
            class _BinaryOperation,
            class _UnaryOperation,
            class _Tp>
  _LIBCPP_HIDE_FROM_ABI optional<_ForwardIterator2> operator()(
      _Policy&&,
      _ForwardIterator1 __first,
      _ForwardIterator1 __last,
      _ForwardIterator2 __result,
      _BinaryOperation __reduce,
      _UnaryOperation __transform,
      _Tp __init) const noexcept {
    return std::transform_inclusive_scan(
        std::move(__first),
        std::move(__last),
        std::move(__result),
        std::move(__reduce),
        std::move(__transform),
        std::move(__init));
  }
};

template <class _ExecutionPolicy>
struct __transform_reduce<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator, class _Tp, class _BinaryOperation, class _UnaryOperation>
  _LIBCPP_HIDE_FROM_ABI optional<_Tp>
  operator()(_Policy&&,
             _ForwardIterator __first,
             _ForwardIterator __last,
             _Tp __init,
             _BinaryOperation&& __reduce,
             _UnaryOperation&& __transform) const noexcept {
    return std::transform_reduce(
        std::move(__first),
        std::move(__last),
        std::move(__init),
        std::forward<_BinaryOperation>(__reduce),
        std::forward<_UnaryOperation>(__transform));
  }
};

template <class _ExecutionPolicy>
struct __transform_reduce_binary<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy,
            class _ForwardIterator1,
            class _ForwardIterator2,
            class _Tp,
            class _BinaryOperation1,
            class _BinaryOperation2>
  _LIBCPP_HIDE_FROM_ABI optional<_Tp> operator()(
      _Policy&&,
      _ForwardIterator1 __first1,
      _ForwardIterator1 __last1,
      _ForwardIterator2 __first2,
      _Tp __init,
      _BinaryOperation1&& __reduce,
      _BinaryOperation2&& __transform) const noexcept {
    return std::transform_reduce(
        std::move(__first1),
        std::move(__last1),
        std::move(__first2),
        std::move(__init),
        std::forward<_BinaryOperation1>(__reduce),
        std::forward<_BinaryOperation2>(__transform));
  }
};

template <class _ExecutionPolicy>
struct __uninitialized_copy<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _InputIterator, class _ForwardIterator>
  _LIBCPP_HIDE_FROM_ABI optional<_ForwardIterator>
  operator()(_Policy&&, _InputIterator __first, _InputIterator __last, _ForwardIterator __result) const noexcept {
    return std::uninitialized_copy(std::move(__first), std::move(__last), std::move(__result));
  }
};

template <class _ExecutionPolicy>
struct __uninitialized_move<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _InputIterator, class _ForwardIterator>
  _LIBCPP_HIDE_FROM_ABI optional<_ForwardIterator>
  operator()(_Policy&&, _InputIterator __first, _InputIterator __last, _ForwardIterator __result) const noexcept {
    return std::uninitialized_move(std::move(__first), std::move(__last), std::move(__result));
  }
};

template <class _ExecutionPolicy>
struct __includes<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator1, class _ForwardIterator2, class _Compare>
  optional<bool>
  operator()(_Policy&&,
             _ForwardIterator1 __first1,
             _ForwardIterator1 __last1,
             _ForwardIterator2 __first2,
             _ForwardIterator2 __last2,
             _Compare __comp) const noexcept {
    return std::includes(
        std::move(__first1), std::move(__last1), std::move(__first2), std::move(__last2), std::move(__comp));
  }
};

template <class _ExecutionPolicy>
struct __set_difference<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardOutIterator, class _Compare>
  optional<_ForwardOutIterator> operator()(
      _Policy&&,
      _ForwardIterator1 __first1,
      _ForwardIterator1 __last1,
      _ForwardIterator2 __first2,
      _ForwardIterator2 __last2,
      _ForwardOutIterator __result,
      _Compare __comp) const noexcept {
    return std::set_difference(
        std::move(__first1),
        std::move(__last1),
        std::move(__first2),
        std::move(__last2),
        std::move(__result),
        std::move(__comp));
  }
};

template <class _ExecutionPolicy>
struct __set_symmetric_difference<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardOutIterator, class _Compare>
  optional<_ForwardOutIterator> operator()(
      _Policy&&,
      _ForwardIterator1 __first1,
      _ForwardIterator1 __last1,
      _ForwardIterator2 __first2,
      _ForwardIterator2 __last2,
      _ForwardOutIterator __result,
      _Compare __comp) const noexcept {
    return std::set_symmetric_difference(
        std::move(__first1),
        std::move(__last1),
        std::move(__first2),
        std::move(__last2),
        std::move(__result),
        std::move(__comp));
  }
};

template <class _ExecutionPolicy>
struct __set_union<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardOutIterator, class _Compare>
  optional<_ForwardOutIterator> operator()(
      _Policy&&,
      _ForwardIterator1 __first1,
      _ForwardIterator1 __last1,
      _ForwardIterator2 __first2,
      _ForwardIterator2 __last2,
      _ForwardOutIterator __result,
      _Compare __comp) const noexcept {
    return std::set_union(
        std::move(__first1),
        std::move(__last1),
        std::move(__first2),
        std::move(__last2),
        std::move(__result),
        std::move(__comp));
  }
};

template <class _ExecutionPolicy>
struct __set_intersection<__serial_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _ForwardIterator1, class _ForwardIterator2, class _ForwardOutIterator, class _Compare>
  optional<_ForwardOutIterator> operator()(
      _Policy&&,
      _ForwardIterator1 __first1,
      _ForwardIterator1 __last1,
      _ForwardIterator2 __first2,
      _ForwardIterator2 __last2,
      _ForwardOutIterator __result,
      _Compare __comp) const noexcept {
    return std::set_intersection(
        std::move(__first1),
        std::move(__last1),
        std::move(__first2),
        std::move(__last2),
        std::move(__result),
        std::move(__comp));
  }
};

#  endif // defined(__AVM__)

} // namespace __pstl
_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 17

_LIBCPP_POP_MACROS

#endif // _LIBCPP___PSTL_BACKENDS_SERIAL_H
