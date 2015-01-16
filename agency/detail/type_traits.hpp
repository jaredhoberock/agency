#pragma once

#include <type_traits>
#include <tuple>
#include <agency/detail/integer_sequence.hpp>

#define __DEFINE_HAS_NESTED_TYPE(trait_name, nested_type_name) \
template<typename T> \
  struct trait_name  \
{                    \
  typedef char yes_type; \
  typedef int  no_type;  \
  template<typename S> static yes_type test(typename S::nested_type_name *); \
  template<typename S> static no_type  test(...); \
  static bool const value = sizeof(test<T>(0)) == sizeof(yes_type);\
  typedef std::integral_constant<bool, value> type;\
};

namespace agency
{
namespace detail
{


template<bool b, typename T, typename F>
struct lazy_conditional
{
  using type = typename T::type;
};


template<typename T, typename F>
struct lazy_conditional<false,T,F>
{
  using type = typename F::type;
};


template<typename T>
struct identity
{
  typedef T type;
};


template<class T>
using decay_t = typename std::decay<T>::type;


template<class T>
using result_of_t = typename std::result_of<T>::type;


template<class... Types> struct type_list {};


template<class TypeList> struct type_list_size;


template<class... Types>
struct type_list_size<type_list<Types...>> : std::integral_constant<size_t, sizeof...(Types)> {};


template<size_t i, class TypeList>
struct type_list_element_impl;


template<class T0, class... Types>
struct type_list_element_impl<0,type_list<T0,Types...>>
{
  using type = T0;
};


template<size_t i, class T0, class... Types>
struct type_list_element_impl<i,type_list<T0,Types...>>
{
  using type = typename type_list_element_impl<i-1,type_list<Types...>>::type;
};


template<size_t i, class TypeList>
using type_list_element = typename type_list_element_impl<i,TypeList>::type;


template<class TypeList, class IndexSequence>
struct type_list_take_impl_impl;


template<class TypeList, size_t... I>
struct type_list_take_impl_impl<TypeList, index_sequence<I...>>
{
  using type = type_list<
    type_list_element<I,TypeList>...
  >;
};


template<size_t n, class TypeList>
struct type_list_take_impl;


template<size_t n, class... Types>
struct type_list_take_impl<n,type_list<Types...>>
{
  using type = typename type_list_take_impl_impl<
    type_list<Types...>,
    make_index_sequence<n>
  >::type;
};


template<size_t n, class TypeList>
using type_list_take = typename type_list_take_impl<n,TypeList>::type;


namespace type_list_detail
{


template<int a, int b>
struct max : std::integral_constant<
  int,
  (a < b ? b : a)
>
{};


} // end type_list_detail


template<size_t n, class TypeList>
using type_list_drop = type_list_take<
  type_list_detail::max<
    0,
    type_list_size<TypeList>::value - n
  >::value,
  TypeList
>;


template<class TypeList>
using type_list_drop_last = type_list_drop<1,TypeList>;


template<class T, class TypeList>
struct is_constructible_from_type_list;


template<class T, class... Types>
struct is_constructible_from_type_list<T,type_list<Types...>>
  : std::is_constructible<T,Types...>
{};


} // end detail
} // end agency
