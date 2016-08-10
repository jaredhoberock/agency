#pragma once

#include <agency/detail/config.hpp>
#include <agency/detail/type_traits.hpp>
#include <agency/detail/type_list.hpp>
#include <utility>
#include <type_traits>
#include <cstddef>


namespace agency
{
namespace detail
{
namespace callable_parameter_detail
{


template<class T>
struct has_call_operator
{
  template<class U,
           class = decltype(&U::operator())
          >
  static std::true_type test(int);

  template<class U>
  static std::false_type test(...);

  static constexpr bool value = decltype(test<T>(0))::value;
};


} // end callable_parameter_detail


// returns true_type if T is a (concrete) callable; false_type, otherwise
// returns false_type if T is a function with overloads or a template call operator
// XXX this could use a better name considering that std::is_callable means something else
//     has_arity ?
//     is_not_overloaded ?
template<class T>
struct is_callable
  : std::integral_constant<
      bool,
      callable_parameter_detail::has_call_operator<T>::value || std::is_function<T>::value
    >
{};


namespace callable_parameter_detail
{


template<class MemberFunction>
struct member_function_raw_parameter_list;

template<class Result, class T, class... Args>
struct member_function_raw_parameter_list<Result (T::*)(Args...)>
{
  using type = type_list<Args...>;
};

template<class Result, class T, class... Args>
struct member_function_raw_parameter_list<Result (T::*)(Args...) const>
{
  using type = type_list<Args...>;
};

template<class MemberFunction>
using member_function_raw_parameter_list_t = typename member_function_raw_parameter_list<MemberFunction>::type;


template<class Function>
struct function_raw_parameter_list;

template<class Result, class... Args>
struct function_raw_parameter_list<Result(Args...)>
{
  using type = type_list<Args...>;
};

template<class Function>
using function_raw_parameter_list_t = typename function_raw_parameter_list<Function>::type;


template<class T>
struct call_operator_type
{
  using type = decltype(&T::operator());
};

template<class T>
using call_operator_type_t = typename call_operator_type<T>::type;


template<class Function>
struct raw_parameter_list
{
  using type = typename lazy_conditional<
    std::is_member_function_pointer<Function>::value,
    member_function_raw_parameter_list<Function>,
    function_raw_parameter_list<Function>
  >::type;
};

template<class Function>
using raw_parameter_list_t = typename raw_parameter_list<Function>::type;

template<class Function>
struct parameter_list
{
  using raw_parameters = raw_parameter_list_t<Function>;

  using type = type_list_map<std::decay, raw_parameters>;
};


template<class Callable>
struct callable_raw_parameter_list
{
  // get the type of the function whose parameters we want
  using function_type = typename lazy_conditional<
    std::is_class<Callable>::value,  // if Callable is a class (i.e., a functor),
    call_operator_type<Callable>,    // return the type of its call operator
    identity<Callable>               // otherwise, Callable must already be a function type
  >::type;

  // return the function_type's list of raw parameters as a type_list
  using type = raw_parameter_list_t<function_type>;
};

template<class Callable>
using callable_raw_parameter_list_t = typename callable_raw_parameter_list<Callable>::type;


template<std::size_t I, class Callable>
struct callable_raw_parameter
{
  using type = type_list_element<I, callable_raw_parameter_list_t<Callable>>;
};

template<std::size_t I, class Callable>
using callable_raw_parameter_t = typename callable_raw_parameter<I,Callable>::type;


} // end callable_parameter_detail


template<class Callable>
struct callable_parameter_list
{
  using type = type_list_map<std::decay, callable_parameter_detail::callable_raw_parameter_list_t<Callable>>;
};

// returns the types of the given Callable's parameters as a type_list
template<class Callable>
using callable_parameter_list_t = typename callable_parameter_list<Callable>::type;


// returns Callable's parameter list if it is callable,
// otherwise returns Default
template<class Callable, class Default>
using callable_parameter_list_or_t = lazy_conditional_t<
  is_callable<Callable>::value,
  callable_parameter_list<Callable>,
  identity<Default>
>;

template<class Callable>
using callable_parameter_list_or_empty_list_t = callable_parameter_list_or_t<Callable,type_list<>>;


template<std::size_t I, class Callable>
struct callable_parameter
  : std::decay<callable_parameter_detail::callable_raw_parameter_t<I,Callable>>
{};

// returns the type of the Ith parameter of the given Callable
template<std::size_t I, class Callable>
using callable_parameter_t = typename callable_parameter<I,Callable>::type;


// returns the type of the Ith parameter of the given Callable
// return Default if Callable is not a callable, or if the Ith parameter does not exist
template<std::size_t I, class Callable, class Default>
using callable_parameter_or_t = lazy_conditional_t<
  (type_list_size<
    callable_parameter_list_or_empty_list_t<Callable>
  >::value > I),
  callable_parameter<I,Callable>,
  identity<Default>
>;


// returns true_type if Callable's Ith parameter is the same as T; false_type, otherwise
// returns false_type if Callable is not a callable
template<std::size_t I, class Callable, class T>
struct callable_parameter_is_same
  : std::conditional<
      is_callable<Callable>::value,           // if Callable is a callable type
      std::is_same<                           // then return true if its Ith parameter type is the same as T
        callable_parameter_or_t<I,Callable,T>,
        T
      >,
      std::false_type                         // else return false
    >::type
{};


} // end detail
} // end aegncy

