/// \file
/// \brief Include this file to use default_bulk_invoke().
///

#pragma once

#include <agency/detail/config.hpp>
#include <agency/detail/control_structures/bulk_invoke_execution_policy.hpp>
#include <agency/detail/type_traits.hpp>
#include <agency/detail/integer_sequence.hpp>
#include <agency/detail/control_structures/is_bulk_call_possible_via_execution_policy.hpp>
#include <agency/execution/execution_agent.hpp>

namespace agency
{
namespace detail
{


template<bool enable, class ExecutionPolicy, class Function, class... Args>
struct enable_if_bulk_invoke_execution_policy_impl {};

template<class ExecutionPolicy, class Function, class... Args>
struct enable_if_bulk_invoke_execution_policy_impl<true, ExecutionPolicy, Function, Args...>
{
  using type = bulk_invoke_execution_policy_result_t<ExecutionPolicy,Function,Args...>;
};


template<class ExecutionPolicy, class Function, class... Args>
struct enable_if_bulk_invoke_execution_policy
  : enable_if_bulk_invoke_execution_policy_impl<
      is_bulk_call_possible_via_execution_policy<decay_t<ExecutionPolicy>,Function,Args...>::value,
      decay_t<ExecutionPolicy>,
      Function,
      Args...
    >
{};


} // end detail


/// \brief Creates a bulk synchronous invocation.
/// \ingroup control_structures
///
///
/// `default_bulk_invoke` is a control structure which creates a group of function invocations with forward progress ordering as required by an execution policy.
/// The results of these invocations, if any, are collected into a container and returned as default_bulk_invoke's result.
///
/// `default_bulk_invoke` creates a group of function invocations of size `N`, and each invocation i in `[0,N)` has the following form:
///
///     result_i = f(agent_i, arg_i_1, arg_i_2, ..., arg_i_M)
///
/// `agent_i` is a reference to an **execution agent** which identifies the ith invocation within the group.
/// The parameter `arg_i_j` depends on the `M` arguments `arg_j` passed to `default_bulk_invoke`:
///    * If `arg_j` is a **shared parameter**, then it is a reference to an object shared among all execution agents in `agent_i`'s group.
///    * Otherwise, `arg_i_j` is a copy of argument `arg_j`.
///
/// If the invocations of `f` do not return `void`, these results are collected and returned in a container `results`, whose type is implementation-defined.
/// If invocation i returns `result_i`, and this invocation's `agent_i` has index `idx_i`, then `results[idx_i]` yields `result_i`.
///
/// The difference between `default_bulk_invoke` and `bulk_invoke` is that unlike `bulk_invoke`, `default_bulk_invoke` is not a customization point whose
/// behavior can be customized with a fancy execution policy.
///
/// \param policy An execution policy describing the requirements of the execution agents created by this call to `default_bulk_invoke`.
/// \param f      A function defining the work to be performed by execution agents.
/// \param args   Additional arguments to pass to `f` when it is invoked.
/// \return `void`, if `f` has no result; otherwise, a container of `f`'s results indexed by the execution agent which produced them.
///
/// \tparam ExecutionPolicy This type must fulfill the requirements of `ExecutionPolicy`.
/// \tparam Function `Function`'s first parameter type must be `ExecutionPolicy::execution_agent_type&`.
///         The types of its additional parameters must match `Args...`.
/// \tparam Args Each type in `Args...` must match the type of the corresponding parameter of `Function`.
///
/// The following example demonstrates how to use `default_bulk_invoke` to print 10 "Hello, world" messages in sequence.
///
/// \include hello_lambda.cpp
///
/// Messages from agents 0 through 9 are printed in sequential order:
///
/// ~~~~
/// $ clang -std=c++11 -I. -lstdc++ -pthread examples/hello_lambda.cpp -o hello_lambda
/// $ ./hello_lambda
/// Hello, world from agent 0
/// Hello, world from agent 1
/// Hello, world from agent 2
/// Hello, world from agent 3
/// Hello, world from agent 4
/// Hello, world from agent 5
/// Hello, world from agent 6
/// Hello, world from agent 7
/// Hello, world from agent 8
/// Hello, world from agent 9
/// ~~~~
///
/// Changing the execution policy used in the call to `default_bulk_invoke` changes how and where the execution agents
/// will execute the provided function. This example demonstrates how to use `default_bulk_invoke` with `par` to execute
/// the SAXPY operation in parallel:
///
/// \include saxpy.cpp
///
/// Remember to include optimization (`-O3`, in this example) to execute fast:
///
///     $ clang -std=c++11 -I. -lstdc++ -pthread -O3 examples/saxpy.cpp -o saxpy
///     $ ./saxpy 
///     OK
///
/// \see bulk_invoke
/// \see bulk_async
/// \see bulk_then
template<class ExecutionPolicy, class Function, class... Args>
__AGENCY_ANNOTATION
#ifndef DOXYGEN_SHOULD_SKIP_THIS
typename detail::enable_if_bulk_invoke_execution_policy<
  ExecutionPolicy, Function, Args...
>::type
#else
see_below
#endif
  default_bulk_invoke(ExecutionPolicy&& policy, Function f, Args&&... args)
{
  using agent_traits = execution_agent_traits<typename std::decay<ExecutionPolicy>::type::execution_agent_type>;
  const size_t num_shared_params = detail::execution_depth<typename agent_traits::execution_category>::value;

  return detail::bulk_invoke_execution_policy(detail::index_sequence_for<Args...>(), detail::make_index_sequence<num_shared_params>(), policy, f, std::forward<Args>(args)...);
}


} // end agency


