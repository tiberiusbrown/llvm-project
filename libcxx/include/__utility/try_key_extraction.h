//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___UTILITY_TRY_EXTRACT_KEY_H
#define _LIBCPP___UTILITY_TRY_EXTRACT_KEY_H

#include <__config>
#include <__fwd/pair.h>
#include <__fwd/tuple.h>
#include <__type_traits/enable_if.h>
#include <__type_traits/integral_constant.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_const.h>
#include <__type_traits/remove_const_ref.h>
#include <__utility/declval.h>
#include <__utility/forward.h>
#include <__utility/piecewise_construct.h>
#include <__utility/priority_tag.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

// AVM's small stack cannot afford separate callback copies and forwarding
// frames at O0. The entry owns the callbacks; dispatch only borrows them.
#if defined(__AVM__)
#  define _LIBCPP_AVM_KEY_INLINE _LIBCPP_ALWAYS_INLINE
#  define _LIBCPP_AVM_KEY_CALLBACK(_Tp) _Tp&
#else
#  define _LIBCPP_AVM_KEY_INLINE
#  define _LIBCPP_AVM_KEY_CALLBACK(_Tp) _Tp
#endif

template <class _KeyT, bool, class _Ret, class _WithKey, class _WithoutKey, class... _Args>
_LIBCPP_AVM_KEY_INLINE _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Ret __try_key_extraction_impl(
    __priority_tag<0>,
    _LIBCPP_AVM_KEY_CALLBACK(_WithKey),
    _LIBCPP_AVM_KEY_CALLBACK(_WithoutKey) __without_key,
    _Args&&... __args) {
  return __without_key(std::forward<_Args>(__args)...);
}

template <class _KeyT,
          bool,
          class _Ret,
          class _WithKey,
          class _WithoutKey,
          class _Arg,
          __enable_if_t<is_same<_KeyT, __remove_const_ref_t<_Arg> >::value, int> = 0>
_LIBCPP_AVM_KEY_INLINE _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Ret __try_key_extraction_impl(
    __priority_tag<1>,
    _LIBCPP_AVM_KEY_CALLBACK(_WithKey) __with_key,
    _LIBCPP_AVM_KEY_CALLBACK(_WithoutKey),
    _Arg&& __arg) {
  return __with_key(__arg, std::forward<_Arg>(__arg));
}

template <class _KeyT,
          bool __is_map,
          class _Ret,
          class _WithKey,
          class _WithoutKey,
          class _Arg,
          __enable_if_t<__is_map && __is_pair_v<__remove_const_ref_t<_Arg> > &&
                            is_same<__remove_const_t<typename __remove_const_ref_t<_Arg>::first_type>, _KeyT>::value,
                        int> = 0>
_LIBCPP_AVM_KEY_INLINE _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Ret __try_key_extraction_impl(
    __priority_tag<1>,
    _LIBCPP_AVM_KEY_CALLBACK(_WithKey) __with_key,
    _LIBCPP_AVM_KEY_CALLBACK(_WithoutKey),
    _Arg&& __arg) {
  return __with_key(__arg.first, std::forward<_Arg>(__arg));
}

template <class _KeyT,
          bool __is_map,
          class _Ret,
          class _WithKey,
          class _WithoutKey,
          class _Arg1,
          class _Arg2,
          __enable_if_t<__is_map && is_same<_KeyT, __remove_const_ref_t<_Arg1> >::value, int> = 0>
_LIBCPP_AVM_KEY_INLINE _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Ret __try_key_extraction_impl(
    __priority_tag<1>,
    _LIBCPP_AVM_KEY_CALLBACK(_WithKey) __with_key,
    _LIBCPP_AVM_KEY_CALLBACK(_WithoutKey),
    _Arg1&& __arg1,
    _Arg2&& __arg2) {
  return __with_key(__arg1, std::forward<_Arg1>(__arg1), std::forward<_Arg2>(__arg2));
}

#ifndef _LIBCPP_CXX03_LANG
template <class _KeyT,
          bool __is_map,
          class _Ret,
          class _WithKey,
          class _WithoutKey,
          class _PiecewiseConstruct,
          class _Tuple1,
          class _Tuple2,
          __enable_if_t<__is_map && is_same<__remove_const_ref_t<_PiecewiseConstruct>, piecewise_construct_t>::value &&
                            __is_tuple_v<_Tuple1> && tuple_size<_Tuple1>::value == 1 &&
                            is_same<__remove_const_ref_t<typename tuple_element<0, _Tuple1>::type>, _KeyT>::value,
                        int> = 0>
_LIBCPP_AVM_KEY_INLINE _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Ret __try_key_extraction_impl(
    __priority_tag<1>,
    _LIBCPP_AVM_KEY_CALLBACK(_WithKey) __with_key,
    _LIBCPP_AVM_KEY_CALLBACK(_WithoutKey),
    _PiecewiseConstruct&& __pc,
    _Tuple1&& __tuple1,
    _Tuple2&& __tuple2) {
  return __with_key(
      std::get<0>(__tuple1),
      std::forward<_PiecewiseConstruct>(__pc),
      std::forward<_Tuple1>(__tuple1),
      std::forward<_Tuple2>(__tuple2));
}
#endif // _LIBCPP_CXX03_LANG

#if defined(__AVM__)
// Describe extraction without runtime callbacks so hash insertion can keep its
// lookup, construction, and rollback in one frame even at O0.
template <class _KeyT, bool, class, class... _Args>
struct __avm_key_extraction_impl : false_type {};

template <class _KeyT, bool _IsMap, class _Arg>
struct __avm_key_extraction_impl< _KeyT,
                                  _IsMap,
                                  __enable_if_t<is_same<_KeyT, __remove_const_ref_t<_Arg> >::value>,
                                  _Arg> : true_type {
  _LIBCPP_ALWAYS_INLINE _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 static const _KeyT& __get(_Arg&& __arg) {
    return __arg;
  }
};

template <class _KeyT, class _Arg>
struct __avm_key_extraction_impl<
    _KeyT,
    true,
    __enable_if_t<__is_pair_v<__remove_const_ref_t<_Arg> > &&
                  is_same<__remove_const_t<typename __remove_const_ref_t<_Arg>::first_type>, _KeyT>::value>,
    _Arg> : true_type {
  _LIBCPP_ALWAYS_INLINE _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 static const _KeyT& __get(_Arg&& __arg) {
    return __arg.first;
  }
};

template <class _KeyT, class _Arg1, class _Arg2>
struct __avm_key_extraction_impl< _KeyT,
                                  true,
                                  __enable_if_t<is_same<_KeyT, __remove_const_ref_t<_Arg1> >::value>,
                                  _Arg1,
                                  _Arg2> : true_type {
  _LIBCPP_ALWAYS_INLINE _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 static const _KeyT&
  __get(_Arg1&& __arg1, _Arg2&&) {
    return __arg1;
  }
};

#  ifndef _LIBCPP_CXX03_LANG
template <class _KeyT, class _PiecewiseConstruct, class _Tuple1, class _Tuple2>
struct __avm_key_extraction_impl<
    _KeyT,
    true,
    __enable_if_t<is_same<__remove_const_ref_t<_PiecewiseConstruct>, piecewise_construct_t>::value &&
                  __is_tuple_v<_Tuple1> && tuple_size<_Tuple1>::value == 1 &&
                  is_same<__remove_const_ref_t<typename tuple_element<0, _Tuple1>::type>, _KeyT>::value>,
    _PiecewiseConstruct,
    _Tuple1,
    _Tuple2> : true_type {
  _LIBCPP_ALWAYS_INLINE _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 static const _KeyT&
  __get(_PiecewiseConstruct&&, _Tuple1&& __tuple1, _Tuple2&&) {
    return std::get<0>(__tuple1);
  }
};
#  endif // _LIBCPP_CXX03_LANG

template <class _KeyT, class _ValT, class... _Args>
using __avm_key_extraction = __avm_key_extraction_impl<_KeyT, !is_same<_KeyT, _ValT>::value, void, _Args...>;
#endif // __AVM__

// This function tries extracting the given _KeyT from _Args...
// If it succeeds to extract the key, it calls the `__with_key` function with the extracted key and all of the
// arguments. Otherwise it calls the `__without_key` function with all of the arguments.
//
// Both `__with_key` and `__without_key` must take all arguments by reference.
template <class _KeyT, class _ValT, class _WithKey, class _WithoutKey, class... _Args>
_LIBCPP_AVM_KEY_INLINE
    _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 decltype(std::declval<_WithoutKey>()(std::declval<_Args>()...))
    __try_key_extraction(_WithKey __with_key, _WithoutKey __without_key, _Args&&... __args) {
  using _Ret = decltype(__without_key(std::forward<_Args>(__args)...));
  return std::__try_key_extraction_impl<_KeyT, !is_same<_KeyT, _ValT>::value, _Ret>(
      __priority_tag<1>(), __with_key, __without_key, std::forward<_Args>(__args)...);
}

#undef _LIBCPP_AVM_KEY_CALLBACK
#undef _LIBCPP_AVM_KEY_INLINE

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___UTILITY_TRY_EXTRACT_KEY_H
