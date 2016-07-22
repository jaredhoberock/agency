#pragma once

#include <agency/detail/config.hpp>
#include <agency/future.hpp>
#include <agency/executor/executor_traits.hpp>
#include <agency/executor/detail/executor_traits/check_for_member_functions.hpp>
#include <agency/executor/detail/executor_traits/ignore_tail_parameters_and_invoke.hpp>
#include <agency/detail/invoke.hpp>
#include <agency/detail/type_traits.hpp>
#include <type_traits>
#include <utility>


namespace agency
{
namespace detail
{
namespace executor_traits_detail
{
namespace multi_agent_then_execute_returning_user_specified_container_implementation_strategies
{


struct use_multi_agent_then_execute_returning_user_specified_container_member_function {};

struct use_multi_agent_then_execute_with_shared_inits_returning_user_specified_container {};

template<class Executor, class Function, class Factory, class Future>
using select_multi_agent_then_execute_returning_user_specified_container_implementation =
  typename std::conditional<
    has_multi_agent_then_execute_returning_user_specified_container<Executor,Function,Factory,Future>::value,
    use_multi_agent_then_execute_returning_user_specified_container_member_function,
    use_multi_agent_then_execute_with_shared_inits_returning_user_specified_container
  >::type;


__agency_exec_check_disable__
template<class Executor, class Function, class Factory, class Future>
__AGENCY_ANNOTATION
typename executor_traits<Executor>::template future<
  result_of_t<Factory(typename executor_traits<Executor>::shape_type)>
>
  multi_agent_then_execute_returning_user_specified_container(use_multi_agent_then_execute_returning_user_specified_container_member_function,
                                                              Executor& ex, Function f, Factory result_factory, typename executor_traits<Executor>::shape_type shape, Future& fut)
{
  return ex.then_execute(f, result_factory, shape, fut);
} // end multi_agent_then_execute_returning_user_specified_container()


template<class Function, class T>
struct ignore_tail_parameters_and_invoke
{
  mutable Function f;

  template<class Index, class... Args>
  __AGENCY_ANNOTATION
  result_of_t<Function(Index,T&)>
  operator()(const Index& idx, T& past_arg, Args&&...) const
  {
    return agency::detail::invoke(f, idx, past_arg);
  }
};


template<class Function>
struct ignore_tail_parameters_and_invoke<Function,void>
{
  mutable Function f;

  __agency_exec_check_disable__
  template<class Index, class... Args>
  __AGENCY_ANNOTATION
  result_of_t<Function(Index)>
  operator()(const Index& idx, Args&&...) const
  {
    return agency::detail::invoke(f, idx);
  }
};


template<size_t... Indices, class Executor, class Function, class Factory, class Future, class Tuple>
__AGENCY_ANNOTATION
typename executor_traits<Executor>::template future<
  result_of_t<Factory(typename executor_traits<Executor>::shape_type)>
>
  multi_agent_then_execute_returning_user_specified_container_impl(detail::index_sequence<Indices...>,
                                                                   Executor& ex, Function f, Factory result_factory, typename executor_traits<Executor>::shape_type shape, Future& fut,
                                                                   const Tuple& tuple_of_unit_factories)
{
  using value_type = typename future_traits<Future>::value_type;

  return executor_traits<Executor>::then_execute(ex, ignore_tail_parameters_and_invoke<Function,value_type>{f}, result_factory, shape, fut, std::get<Indices>(tuple_of_unit_factories)...);
} // end multi_agent_then_execute_returning_user_specified_container_impl()


template<class Executor, class Function, class Factory, class Future>
__AGENCY_ANNOTATION
typename executor_traits<Executor>::template future<
  result_of_t<Factory(typename executor_traits<Executor>::shape_type)>
>
  multi_agent_then_execute_returning_user_specified_container(use_multi_agent_then_execute_with_shared_inits_returning_user_specified_container,
                                                              Executor& ex, Function f, Factory result_factory, typename executor_traits<Executor>::shape_type shape, Future& fut)
{
  auto tuple_of_unit_factories = executor_traits_detail::make_tuple_of_unit_factories(ex);

  return multi_agent_then_execute_returning_user_specified_container_impl(detail::make_index_sequence<std::tuple_size<decltype(tuple_of_unit_factories)>::value>(), ex, f, result_factory, shape, fut, tuple_of_unit_factories);
} // end multi_agent_then_execute_returning_user_specified_container()


} // end multi_agent_then_execute_returning_user_specified_container_implementation_strategies
} // end executor_traits_detail
} // end detail


template<class Executor>
  template<class Function, class Future, class Factory,
           class Enable1,
           class Enable2
           >
__AGENCY_ANNOTATION
typename executor_traits<Executor>::template future<
  detail::result_of_t<Factory(typename executor_traits<Executor>::shape_type)>
>
  executor_traits<Executor>
    ::then_execute(typename executor_traits<Executor>::executor_type& ex,
                   Function f,
                   Factory result_factory,
                   typename executor_traits<Executor>::shape_type shape,
                   Future& fut)
{
  namespace ns = detail::executor_traits_detail::multi_agent_then_execute_returning_user_specified_container_implementation_strategies;

  using check_for_member_function = ns::select_multi_agent_then_execute_returning_user_specified_container_implementation<Executor,Function,Factory,Future>;

  return ns::multi_agent_then_execute_returning_user_specified_container(check_for_member_function(), ex, f, result_factory, shape, fut);
} // end executor_traits::then_execute()


} // end agency

