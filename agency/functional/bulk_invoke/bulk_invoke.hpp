/// \file
/// \brief Include this file to use bulk_invoke().
///

#pragma once

#include <agency/detail/config.hpp>
#include <agency/functional/bulk_invoke/default_bulk_invoke.hpp>
#include <agency/functional/invoke.hpp>
#include <agency/functional/detail/customization_point.hpp>
#include <agency/detail/static_const.hpp>


namespace agency
{
namespace detail
{


// agency::bulk_invoke is a *customization point object* which allows its users to provide a custom implementation
// which conforms to bulk_invoke's semantics.
// these functors define the possible implementations of the bulk_invoke customization point
// the first parameter passed to each of these functors is the bulk_invoke customization point itself


struct call_member_function_bulk_invoke
{
  __agency_exec_check_disable__
  template<class BulkInvoke, class Arg1, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(BulkInvoke&&, Arg1&& arg1, Args&&... args) const ->
    decltype(std::forward<Arg1>(arg1).bulk_invoke(std::forward<Args>(args)...))
  {
    return std::forward<Arg1>(arg1).for_each(std::forward<Args>(args)...);
  }
};


struct call_free_function_bulk_invoke_or_invoke_customization_point
{
  __agency_exec_check_disable__
  template<class BulkInvoke, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(BulkInvoke&&, Args&&... args) const ->
    decltype(bulk_invoke(std::forward<Args>(args)...))
  {
    return bulk_invoke(std::forward<Args>(args)...);
  }

  template<class BulkInvoke, class Arg1, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(BulkInvoke&& customization_point, Arg1&& arg1, Args&&... args) const ->
    decltype(agency::invoke(std::forward<Arg1>(arg1), std::forward<BulkInvoke>(customization_point), std::forward<Args>(args)...))
  {
    return agency::invoke(std::forward<Arg1>(arg1), std::forward<BulkInvoke>(customization_point), std::forward<Args>(args)...);
  }
};


struct call_default_bulk_invoke
{
  __agency_exec_check_disable__
  template<class BulkInvoke, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(BulkInvoke&&, Args&&... args) const ->
    decltype(agency::default_bulk_invoke(std::forward<Args>(args)...))
  {
    return agency::default_bulk_invoke(std::forward<Args>(args)...);
  }
};


struct bulk_invoke_t : customization_point<
  bulk_invoke_t,
  call_member_function_bulk_invoke,
  call_free_function_bulk_invoke_or_invoke_customization_point,
  call_default_bulk_invoke
>
{};


} // end detail


/// \brief Creates a (customizable) bulk synchronous invocation.
/// \ingroup control_structures
///
///
/// `bulk_invoke` is a control structure which creates a group of function invocations with forward progress ordering as required by an execution policy.
/// The results of these invocations, if any, are collected into a container and returned as bulk_invoke's result.
///
/// `bulk_invoke` creates a group of function invocations of size `N`, and each invocation i in `[0,N)` has the following form:
///
///     result_i = f(agent_i, arg_i_1, arg_i_2, ..., arg_i_M)
///
/// `agent_i` is a reference to an **execution agent** which identifies the ith invocation within the group.
/// The parameter `arg_i_j` depends on the `M` arguments `arg_j` passed to `bulk_invoke`:
///    * If `arg_j` is a **shared parameter**, then it is a reference to an object shared among all execution agents in `agent_i`'s group.
///    * Otherwise, `arg_i_j` is a copy of argument `arg_j`.
///
/// If the invocations of `f` do not return `void`, these results are collected and returned in a container `results`, whose type is implementation-defined.
/// If invocation i returns `result_i`, and this invocation's `agent_i` has index `idx_i`, then `results[idx_i]` yields `result_i`.
///
/// The difference between `bulk_invoke` and `default_bulk_invoke` is that unlike `default_bulk_invoke`, `bulk_invoke` is a customization point whose
/// behavior can be customized with a fancy execution policy.
///
/// \param policy An execution policy describing the requirements of the execution agents created by this call to `bulk_invoke`.
/// \param f      A function defining the work to be performed by execution agents.
/// \param args   Additional arguments to pass to `f` when it is invoked.
/// \return `void`, if `f` has no result; otherwise, a container of `f`'s results indexed by the execution agent which produced them.
///
/// \tparam ExecutionPolicy This type must fulfill the requirements of `ExecutionPolicy`.
/// \tparam Function `Function`'s first parameter type must be `ExecutionPolicy::execution_agent_type&`.
///         The types of its additional parameters must match `Args...`.
/// \tparam Args Each type in `Args...` must match the type of the corresponding parameter of `Function`.
///
/// The following example demonstrates how to use `bulk_invoke` to print 10 "Hello, world" messages in sequence.
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
/// Changing the execution policy used in the call to `bulk_invoke` changes how and where the execution agents
/// will execute the provided function. This example demonstrates how to use `bulk_invoke` with `par` to execute
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
/// \see default_bulk_invoke
/// \see bulk_async
/// \see bulk_then

#ifdef DOXYGEN_ONLY
template<class ExecutionPolicy, class Function, class... Args>
__AGENCY_ANNOTATION
see_below bulk_invoke(ExecutionPolicy&& policy, Function f, Args&&... args);
#else
namespace
{

#ifndef __CUDA_ARCH__
constexpr auto const& bulk_invoke = detail::static_const<detail::bulk_invoke_t>::value;
#else
// __device__ functions cannot access global variables, so make bulk_invoke a __device__ variable in __device__ code
const __device__ detail::bulk_invoke_t bulk_invoke;
#endif

} // end anonymous namespace
#endif


} // end agency

