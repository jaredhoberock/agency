/// \file
/// \brief Include this file to use bulk_async().
///

#pragma once

#include <agency/detail/config.hpp>
#include <agency/functional/bulk_async/default_bulk_async.hpp>
#include <agency/functional/invoke.hpp>
#include <agency/functional/detail/customization_point.hpp>
#include <agency/detail/static_const.hpp>


namespace agency
{
namespace detail
{


// agency::bulk_async is a *customization point object* which allows its users to provide a custom implementation
// which conforms to bulk_async's semantics.
// these functors define the possible implementations of the bulk_async customization point
// the first parameter passed to each of these functors is the bulk_async customization point itself


struct call_member_function_bulk_async
{
  __agency_exec_check_disable__
  template<class BulkInvoke, class Arg1, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(BulkInvoke&&, Arg1&& arg1, Args&&... args) const ->
    decltype(std::forward<Arg1>(arg1).bulk_async(std::forward<Args>(args)...))
  {
    return std::forward<Arg1>(arg1).bulk_async(std::forward<Args>(args)...);
  }
};


struct call_free_function_bulk_async_or_invoke_customization_point
{
  __agency_exec_check_disable__
  template<class BulkAsync, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(BulkAsync&&, Args&&... args) const ->
    decltype(bulk_async(std::forward<Args>(args)...))
  {
    return bulk_async(std::forward<Args>(args)...);
  }

  template<class BulkAsync, class Arg1, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(BulkAsync&& customization_point, Arg1&& arg1, Args&&... args) const ->
    decltype(agency::invoke(std::forward<Arg1>(arg1), std::forward<BulkAsync>(customization_point), std::forward<Args>(args)...))
  {
    return agency::invoke(std::forward<Arg1>(arg1), std::forward<BulkAsync>(customization_point), std::forward<Args>(args)...);
  }
};


struct call_default_bulk_async
{
  __agency_exec_check_disable__
  template<class BulkAsync, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(BulkAsync&&, Args&&... args) const ->
    decltype(agency::default_bulk_async(std::forward<Args>(args)...))
  {
    return agency::default_bulk_async(std::forward<Args>(args)...);
  }
};


struct bulk_async_t : customization_point<
  bulk_async_t,
  call_member_function_bulk_async,
  call_free_function_bulk_async_or_invoke_customization_point,
  call_default_bulk_async
>
{};


} // end detail


/// \brief Creates a (customizable) bulk asynchronous invocation.
/// \ingroup control_structures
///
///
/// `bulk_async` is a control structure which asynchronously creates a group of function invocations with forward progress ordering as required by an execution policy.
/// The results of these invocations, if any, are collected into a container and returned as `bulk_async`'s asynchronous result.
/// A future object corresponding to the eventual availability of this container is returned as `bulk_async`'s result.
///
/// `bulk_async` asynchronously creates a group of function invocations of size `N`, and each invocation i in `[0,N)` has the following form:
///
///     result_i = f(agent_i, arg_i_1, arg_i_2, ..., arg_i_M)
///
/// `agent_i` is a reference to an **execution agent** which identifies the ith invocation within the group.
/// The parameter `arg_i_j` depends on the `M` arguments `arg_j` passed to `bulk_async`:
///    * If `arg_j` is a **shared parameter**, then it is a reference to an object shared among all execution agents in `agent_i`'s group.
///    * Otherwise, `arg_i_j` is a copy of argument `arg_j`.
///
/// If the invocations of `f` do not return `void`, these results are collected and returned in a container `results`, whose type is implementation-defined.
/// If invocation i returns `result_i`, and this invocation's `agent_i` has index `idx_i`, then `results[idx_i]` yields `result_i`.
///
/// The difference between `bulk_async` and `default_bulk_async` is that unlike `default_bulk_async`, `bulk_async` is a customization point whose
/// behavior can be customized with a fancy execution policy.
///
/// \param policy An execution policy describing the requirements of the execution agents created by this call to `bulk_async`.
/// \param f      A function defining the work to be performed by execution agents.
/// \param args   Additional arguments to pass to `f` when it is invoked.
/// \return A `void` future object, if `f` has no result; otherwise, a future object corresponding to the eventually available container of `f`'s results indexed by the execution agent which produced them.
/// \note The type of future object returned by `bulk_async` is a property of the type of `ExecutionPolicy` used as a parameter.
///
/// \tparam ExecutionPolicy This type must fulfill the requirements of `ExecutionPolicy`.
/// \tparam Function `Function`'s first parameter type must be `ExecutionPolicy::execution_agent_type&`.
///         The types of its additional parameters must match `Args...`.
/// \tparam Args Each type in `Args...` must match the type of the corresponding parameter of `Function`.
///
/// The following example demonstrates how to use `bulk_async` to create tasks which execute asynchronously with the caller.
///
/// \include hello_async.cpp
///
/// Messages from the agents in the two asynchronous tasks are printed while the main thread sleeps:
///
/// ~~~~
/// $ clang -std=c++11 -I. -lstdc++ -pthread examples/hello_async.cpp -o hello_async
/// $ ./hello_async
/// Starting two tasks asynchronously...
/// Sleeping before waiting on the tasks...
/// Hello, world from agent 0 in task 1
/// Hello, world from agent 1 in task 1
/// Hello, world from agent 2 in task 1
/// Hello, world from agent 3 in task 1
/// Hello, world from agent 4 in task 1
/// Hello, world from agent 0 in task 2
/// Hello, world from agent 1 in task 2
/// Hello, world from agent 2 in task 2
/// Hello, world from agent 3 in task 2
/// Hello, world from agent 4 in task 2
/// Woke up, waiting for the tasks to complete...
/// OK
/// ~~~~
///
/// \see bulk_async
/// \see bulk_invoke
/// \see bulk_then

#ifdef DOXYGEN_ONLY
template<class ExecutionPolicy, class Function, class... Args>
__AGENCY_ANNOTATION
see_below bulk_async(ExecutionPolicy&& policy, Function f, Args&&... args);
#else
namespace
{

#ifndef __CUDA_ARCH__
constexpr auto const& bulk_async = detail::static_const<detail::bulk_async_t>::value;
#else
// __device__ functions cannot access global variables, so make bulk_async a __device__ variable in __device__ code
const __device__ detail::bulk_async_t bulk_async;
#endif

} // end anonymous namespace
#endif


} // end agency

