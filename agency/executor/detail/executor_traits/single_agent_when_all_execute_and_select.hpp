#pragma once

#include <agency/executor/executor_traits.hpp>
#include <agency/executor/detail/executor_traits/check_for_member_functions.hpp>
#include <agency/executor/detail/executor_traits/copy_constructible_function.hpp>
#include <agency/detail/shape_cast.hpp>
#include <agency/future.hpp>
#include <agency/detail/select.hpp>
#include <agency/detail/invoke.hpp>
#include <type_traits>
#include <iostream>

namespace agency
{
namespace detail
{
namespace executor_traits_detail
{
namespace single_agent_when_all_execute_and_select_implementation_strategies
{


// strategies for single_agent_when_all_execute_and_select implementation
struct use_single_agent_when_all_execute_and_select_member_function {};

struct use_multi_agent_when_all_execute_and_select_member_function {};

struct use_when_all_and_single_agent_then_execute {};


template<class Executor, class Function, class TupleOfFutures, size_t... Indices>
using select_single_agent_when_all_execute_and_select_implementation = 
  typename std::conditional<
      has_single_agent_when_all_execute_and_select<Executor, Function, TupleOfFutures, Indices...>::value,
      use_single_agent_when_all_execute_and_select_member_function,
      typename std::conditional<
        has_multi_agent_when_all_execute_and_select<Executor, Function, TupleOfFutures, Indices...>::value,
        use_multi_agent_when_all_execute_and_select_member_function,
        use_when_all_and_single_agent_then_execute
      >::type
    >::type;


template<size_t... Indices, class Executor, class Function, class TupleOfFutures>
typename executor_traits<Executor>::template future<
  detail::when_all_execute_and_select_result_t<
    detail::index_sequence<Indices...>,
    typename std::decay<TupleOfFutures>::type
  >
>
  single_agent_when_all_execute_and_select(use_single_agent_when_all_execute_and_select_member_function, Executor& ex, Function&& f, TupleOfFutures&& futures)
{
  return ex.template when_all_execute_and_select<Indices...>(std::forward<Function>(f), std::forward<TupleOfFutures>(futures));
} // end single_agent_when_all_execute_and_select()


// this functor can turn a move-only function into a copiable function
// we do this when lowering a call to single-agent when_all_execute_and_select()
// to multi-agent when_all_execute_and_select() below
template<class Function>
struct single_agent_when_all_execute_and_select_functor
{
  mutable copy_constructible_function_t<Function> f;

  template<class Function1,
           class = typename std::enable_if<
             std::is_constructible<
               copy_constructible_function_t<Function>,
               Function1&&
             >::value
           >::type>
  __AGENCY_ANNOTATION
  single_agent_when_all_execute_and_select_functor(Function1&& function)
    : f(std::forward<Function1>(function))
  {}

  template<class Index, class... Args>
  __AGENCY_ANNOTATION
  void operator()(const Index&, Args&&... args) const
  {
    agency::detail::invoke(f, std::forward<Args>(args)...);
  }
};


template<size_t... Indices, class Executor, class Function, class TupleOfFutures>
typename executor_traits<Executor>::template future<
  detail::when_all_execute_and_select_result_t<
    detail::index_sequence<Indices...>,
    typename std::decay<TupleOfFutures>::type
  >
>
  single_agent_when_all_execute_and_select(use_multi_agent_when_all_execute_and_select_member_function, Executor& ex, Function&& f, TupleOfFutures&& futures)
{
  // create a multi-agent task with only a single agent
  using shape_type = typename executor_traits<Executor>::shape_type;
  using index_type = typename executor_traits<Executor>::index_type;

  auto g = single_agent_when_all_execute_and_select_functor<typename std::decay<Function>::type>(std::forward<Function>(f));

  return ex.template when_all_execute_and_select<Indices...>(g, detail::shape_cast<shape_type>(1), std::forward<TupleOfFutures>(futures));
} // end single_agent_when_all_execute_and_select()


template<class IndexSequence, class TupleOfFutures>
struct when_all_from_tuple_result_impl;

template<size_t... Indices, class TupleOfFutures>
struct when_all_from_tuple_result_impl<index_sequence<Indices...>, TupleOfFutures>
  : when_all_result<
      typename std::tuple_element<
        Indices,
        TupleOfFutures
      >::type...
    >
{};

template<class TupleOfFutures>
struct when_all_from_tuple_result : when_all_from_tuple_result_impl<
  make_index_sequence<std::tuple_size<TupleOfFutures>::value>,
  TupleOfFutures
>
{};

template<class TupleOfFutures>
using when_all_from_tuple_result_t = typename when_all_from_tuple_result<TupleOfFutures>::type;


template<size_t... Indices, class Executor, class TupleOfFutures>
typename executor_traits<Executor>::template future<
  when_all_from_tuple_result_t<
    typename std::decay<TupleOfFutures>::type
  >
>
  when_all_from_tuple_impl(index_sequence<Indices...>, Executor& ex, TupleOfFutures&& futures)
{
  return executor_traits<Executor>::when_all(ex, std::get<Indices>(std::forward<TupleOfFutures>(futures))...);
}


template<class Executor, class TupleOfFutures>
typename executor_traits<Executor>::template future<
  when_all_from_tuple_result_t<
    typename std::decay<TupleOfFutures>::type
  >
>
  when_all_from_tuple(Executor& ex, TupleOfFutures&& futures)
{
  constexpr size_t num_futures = std::tuple_size<
    typename std::decay<TupleOfFutures>::type
  >::value;

  return when_all_from_tuple_impl(detail::make_index_sequence<num_futures>(), ex, std::forward<TupleOfFutures>(futures));
}


template<size_t num_nonvoid_arguments, class Function, class IndexSequence>
struct invoke_and_select;


template<size_t num_nonvoid_arguments, class Function, size_t... SelectedIndices>
struct invoke_and_select<num_nonvoid_arguments, Function, index_sequence<SelectedIndices...>>
{
  mutable Function f;

  template<size_t... Indices, class Tuple>
  __AGENCY_ANNOTATION
  void invoke_f_and_ignore_result_impl(index_sequence<Indices...>, Tuple& args) const
  {
    f(detail::get<Indices>(args)...);
  }

  template<class Tuple>
  __AGENCY_ANNOTATION
  void invoke_f_and_ignore_result(Tuple& args) const
  {
    invoke_f_and_ignore_result_impl(detail::make_tuple_indices(args), args);
  }

  // 0-element Tuples unwrap to void
  template<class Tuple,
           class = typename std::enable_if<
             (std::tuple_size<
                typename std::decay<Tuple>::type
              >::value == 0)
           >::type>
  __AGENCY_ANNOTATION
  static void unwrap_and_move(Tuple&&) {}

  // 1-element Tuples unwrap and move to their first element
  template<class Tuple,
           class = typename std::enable_if<
             (std::tuple_size<
                typename std::decay<Tuple>::type
              >::value == 1)
           >::type>
  __AGENCY_ANNOTATION
  static typename std::decay<
    typename std::tuple_element<
      0,
      typename std::decay<Tuple>::type
    >::type
  >::type
    unwrap_and_move(Tuple&& t)
  {
    return std::get<0>(std::move(t));
  }

  // n-element Tuples unwrap and move to a tuple of values 
  template<class Tuple,
           class = typename std::enable_if<
             (std::tuple_size<
                typename std::decay<Tuple>::type
              >::value > 1)
           >::type>
  __AGENCY_ANNOTATION
  static decay_tuple_t<Tuple>
    unwrap_and_move(Tuple&& t)
  {
    return decay_tuple_t<Tuple>(std::move(t));
  }


  // usually, we receive all the arguments to f in a tuple
  template<class Tuple>
  __AGENCY_ANNOTATION
  auto operator()(Tuple& args) const
    -> decltype(
         unwrap_and_move(
           detail::select_from_tuple<SelectedIndices...>(args)
         )
       )
  {
    invoke_f_and_ignore_result(args);

    // get the selection from the tuple of arguments
    auto selection = detail::select_from_tuple<SelectedIndices...>(std::move(args));

    // unwrap the selection
    return unwrap_and_move(std::move(selection));
  }
};


template<class Function, size_t... SelectedIndices>
struct invoke_and_select<0, Function, index_sequence<SelectedIndices...>>
{
  mutable Function f;

  // when there are no arguments to f, there is nothing to receive
  __AGENCY_ANNOTATION
  void operator()() const
  {
    f();

    return detail::select<SelectedIndices...>();
  }
};


template<class Function, size_t... SelectedIndices>
struct invoke_and_select<1, Function, index_sequence<SelectedIndices...>>
{
  mutable Function f;

  // when there is only one argument for f, we don't receive a tuple
  // since select() returns a reference, we decay it to a value when we return
  template<class Arg>
  __AGENCY_ANNOTATION
  decay_if_not_void_t<
    select_result_t<index_sequence<SelectedIndices...>, Arg&&>
  >
    operator()(Arg& arg) const
  {
    // invoke f
    agency::detail::invoke(f, arg);

    // return a selection from the argument (either ignore it, or move it along)
    return detail::select<SelectedIndices...>(std::move(arg));
  }
};


template<size_t num_nonvoid_arguments, class IndexSequence, class Function>
__AGENCY_ANNOTATION
invoke_and_select<num_nonvoid_arguments, typename std::decay<Function>::type, IndexSequence>
  make_invoke_and_select(Function&& f)
{
  return invoke_and_select<num_nonvoid_arguments, typename std::decay<Function>::type, IndexSequence>{std::forward<Function>(f)};
}


template<size_t... Indices, class Executor, class Function, class TupleOfFutures>
typename executor_traits<Executor>::template future<
  detail::when_all_execute_and_select_result_t<
    detail::index_sequence<Indices...>,
    typename std::decay<TupleOfFutures>::type
  >
>
  single_agent_when_all_execute_and_select(use_when_all_and_single_agent_then_execute, Executor& ex, Function&& f, TupleOfFutures&& futures)
{
  // join the futures into a single one
  auto fut = when_all_from_tuple(ex, std::forward<TupleOfFutures>(futures));

  // count the number of non-void arguments to f
  using future_types = tuple_elements<typename std::decay<TupleOfFutures>::type>;
  using value_types  = type_list_map<future_value, future_types>;
  using nonvoid_value_types = type_list_filter<is_not_void, value_types>;
  constexpr size_t num_nonvoid_arguments = type_list_size<nonvoid_value_types>::value;

  // map Indices... to corresponding indices of the filtered argument list
  // pass those remapped indices to invoke_and_select
  using flag_sequence = type_list_index_map<is_not_void, value_types>;
  using scanned_flags = exclusive_scan_index_sequence<0, flag_sequence>;
  using mapped_indices = index_sequence_gather<index_sequence<Indices...>, scanned_flags>;

  // create a functor which will pass the arguments to f and return a selection of those arguments
  auto g = make_invoke_and_select<num_nonvoid_arguments, mapped_indices>(std::forward<Function>(f));

  return executor_traits<Executor>::then_execute(ex, std::move(g), fut);
} // end single_agent_when_all_execute_and_select()


} // end single_agent_when_all_execute_and_select_implementation_strategies
} // end executor_traits_detail
} // end detail



template<class Executor>
template<size_t... Indices, class Function, class TupleOfFutures>
  typename executor_traits<Executor>::template future<
    detail::when_all_execute_and_select_result_t<
      detail::index_sequence<Indices...>,
      typename std::decay<TupleOfFutures>::type
    >
  >
  executor_traits<Executor>
    ::when_all_execute_and_select(typename executor_traits<Executor>::executor_type& ex,
                                  Function&& f,
                                  TupleOfFutures&& futures)
{
  namespace ns = detail::executor_traits_detail::single_agent_when_all_execute_and_select_implementation_strategies;

  using implementation_strategy = ns::select_single_agent_when_all_execute_and_select_implementation<
    Executor,
    Function,
    typename std::decay<TupleOfFutures>::type,
    Indices...
  >;

  return ns::single_agent_when_all_execute_and_select<Indices...>(implementation_strategy(), ex, std::forward<Function>(f), std::forward<TupleOfFutures>(futures));
} // end executor_traits::when_all_execute_and_select()


} // end agency


