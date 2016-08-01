#pragma once

#include <agency/detail/config.hpp>
#include <agency/execution/executor/executor_traits.hpp>
#include <agency/detail/integer_sequence.hpp>
#include <agency/detail/tuple.hpp>
#include <agency/detail/bulk_functions/executor_functions/bind_agent_local_parameters.hpp>
#include <agency/detail/bulk_functions/executor_functions/unpack_shared_parameters_from_executor_and_invoke.hpp>
#include <agency/detail/bulk_functions/scope_result.hpp>
#include <agency/detail/bulk_functions/result_factory.hpp>
#include <agency/detail/bulk_functions/decay_parameter.hpp>
#include <agency/detail/type_traits.hpp>
#include <type_traits>

namespace agency
{
namespace detail
{


// this overload handles the general case where the user function returns a normal result
template<class Executor, class Function, class Factory, class Tuple, size_t... TupleIndices>
result_of_t<Factory(executor_shape_t<Executor>)>
  bulk_invoke_executor_impl(Executor& exec,
                            Function f,
                            Factory result_factory,
                            typename executor_traits<Executor>::shape_type shape,
                            Tuple&& factory_tuple,
                            detail::index_sequence<TupleIndices...>)
{
  return executor_traits<Executor>::execute(exec, f, result_factory, shape, detail::get<TupleIndices>(std::forward<Tuple>(factory_tuple))...);
}

// this overload handles the special case where the user function returns a scope_result
template<class Executor, class Function, size_t scope, class T, class Tuple, size_t... TupleIndices>
typename detail::scope_result_container<scope,T,Executor>::result_type
  bulk_invoke_executor_impl(Executor& exec,
                            Function f,
                            detail::executor_traits_detail::container_factory<detail::scope_result_container<scope,T,Executor>> result_factory,
                            typename executor_traits<Executor>::shape_type shape,
                            Tuple&& factory_tuple,
                            detail::index_sequence<TupleIndices...>)
{
  return executor_traits<Executor>::execute(exec, f, result_factory, shape, detail::get<TupleIndices>(std::forward<Tuple>(factory_tuple))...);
}

// this overload handles the special case where the user function returns void
template<class Executor, class Function, class Tuple, size_t... TupleIndices>
void bulk_invoke_executor_impl(Executor& exec,
                               Function f,
                               void_factory,
                               typename executor_traits<Executor>::shape_type shape,
                               Tuple&& factory_tuple,
                               detail::index_sequence<TupleIndices...>)
{
  return executor_traits<Executor>::execute(exec, f, shape, detail::get<TupleIndices>(std::forward<Tuple>(factory_tuple))...);
}


// computes the result type of bulk_invoke(executor)
template<class Executor, class Function, class... Args>
struct bulk_invoke_executor_result
{
  // first figure out what type the user function returns
  using user_function_result = result_of_t<
    Function(executor_index_t<Executor>, decay_parameter_t<Args>...)
  >;

  // if the user function returns scope_result, then use scope_result_to_bulk_invoke_result to figure out what to return
  // else, the result is whatever executor_result<Executor, function_result> thinks it is
  using type = typename detail::lazy_conditional<
    detail::is_scope_result<user_function_result>::value,
    detail::scope_result_to_bulk_invoke_result<user_function_result, Executor>,
    executor_result<Executor, user_function_result>
  >::type;
};

template<class Executor, class Function, class... Args>
using bulk_invoke_executor_result_t = typename bulk_invoke_executor_result<Executor,Function,Args...>::type;


template<class Executor, class Function, class... Args>
bulk_invoke_executor_result_t<Executor, Function, Args...>
  bulk_invoke_executor(Executor& exec, typename executor_traits<Executor>::shape_type shape, Function f, Args&&... args)
{
  // the _1 is for the executor idx parameter, which is the first parameter passed to f
  auto g = detail::bind_agent_local_parameters_workaround_nvbug1754712(std::integral_constant<size_t,1>(), f, detail::placeholders::_1, std::forward<Args>(args)...);

  // make a tuple of the shared args
  auto shared_arg_tuple = detail::forward_shared_parameters_as_tuple(std::forward<Args>(args)...);

  using traits = executor_traits<Executor>;

  // package up the shared parameters for the executor
  const size_t execution_depth = traits::execution_depth;

  // create a tuple of factories to use for shared parameters for the executor
  auto factory_tuple = detail::make_shared_parameter_factory_tuple<execution_depth>(shared_arg_tuple);

  // unpack shared parameters we receive from the executor
  auto h = detail::make_unpack_shared_parameters_from_executor_and_invoke(g);

  // compute the type of f's result
  using result_of_f = result_of_t<Function(executor_index_t<Executor>,decay_parameter_t<Args>...)>;

  // based on the type of f's result, make a factory that will create the appropriate type of container to store f's results
  auto result_factory = detail::make_result_factory<result_of_f>(exec);

  return detail::bulk_invoke_executor_impl(exec, h, result_factory, shape, factory_tuple, detail::make_index_sequence<execution_depth>());
}


} // end detail
} // end agency

