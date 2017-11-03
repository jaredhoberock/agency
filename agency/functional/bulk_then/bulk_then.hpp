/// \file
/// \brief Include this file to use bulk_then().
///

#pragma once

#include <agency/detail/config.hpp>
#include <agency/functional/bulk_then/default_bulk_then.hpp>
#include <agency/functional/invoke.hpp>
#include <agency/functional/detail/customization_point.hpp>
#include <agency/detail/static_const.hpp>


namespace agency
{
namespace detail
{


// agency::bulk_then is a *customization point object* which allows its users to provide a custom implementation
// which conforms to bulk_then's semantics.
// these functors define the possible implementations of the bulk_then customization point
// the first parameter passed to each of these functors is the bulk_then customization point itself


struct call_member_function_bulk_then
{
  __agency_exec_check_disable__
  template<class BulkThen, class Arg1, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(BulkThen&&, Arg1&& arg1, Args&&... args) const ->
    decltype(std::forward<Arg1>(arg1).bulk_then(std::forward<Args>(args)...))
  {
    return std::forward<Arg1>(arg1).bulk_then(std::forward<Args>(args)...);
  }
};


struct call_free_function_bulk_then_or_invoke_customization_point
{
  __agency_exec_check_disable__
  template<class BulkThen, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(BulkThen&&, Args&&... args) const ->
    decltype(bulk_then(std::forward<Args>(args)...))
  {
    return bulk_then(std::forward<Args>(args)...);
  }

  template<class BulkThen, class Arg1, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(BulkThen&& customization_point, Arg1&& arg1, Args&&... args) const ->
    decltype(agency::invoke(std::forward<Arg1>(arg1), std::forward<BulkThen>(customization_point), std::forward<Args>(args)...))
  {
    return agency::invoke(std::forward<Arg1>(arg1), std::forward<BulkThen>(customization_point), std::forward<Args>(args)...);
  }
};


struct call_default_bulk_then
{
  __agency_exec_check_disable__
  template<class BulkThen, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(BulkThen&&, Args&&... args) const ->
    decltype(agency::default_bulk_then(std::forward<Args>(args)...))
  {
    return agency::default_bulk_then(std::forward<Args>(args)...);
  }
};


struct bulk_then_t : customization_point<
  bulk_then_t,
  call_member_function_bulk_then,
  call_free_function_bulk_then_or_invoke_customization_point,
  call_default_bulk_then
>
{};


} // end detail


/// \brief Creates a (customizable) bulk continuation.
/// \ingroup control_structures
///
///
/// `bulk_then` is a control structure which asynchronously creates a group of function invocations with forward progress ordering as required by an execution policy.
/// These invocations are a *bulk continuation* to a predecessor bulk asynchronous invocation. The predecessor bulk asynchronous invocation is represented by a
/// future object, and the continuation will not execute until the predecessor's future object becomes ready.
/// The results of the continuation's invocations, if any, are collected into a container and returned as `bulk_then`'s asynchronous result.
/// A future object corresponding to the eventual availability of this container is returned as `bulk_then`'s result.
///
/// `bulk_then` asynchronously creates a group of function invocations of size `N`, and each invocation i in `[0,N)` has the following form:
///
///     result_i = f(agent_i, predecessor_arg, arg_i_1, arg_i_2, ..., arg_i_M)
///
/// `agent_i` is a reference to an **execution agent** which identifies the ith invocation within the group.
/// The parameter `predecessor_arg` is a reference to the value of the future object used as a parameter to `bulk_then`. If this future object has a `void` value, then this parameter is omitted.
/// The parameter `arg_i_j` depends on the `M` arguments `arg_j` passed to `bulk_invoke`:
///    * If `arg_j` is a **shared parameter**, then it is a reference to an object shared among all execution agents in `agent_i`'s group.
///    * Otherwise, `arg_i_j` is a copy of argument `arg_j`.
///
/// If the invocations of `f` do not return `void`, these results are collected and returned in a container `results`, whose type is implementation-defined.
/// If invocation i returns `result_i`, and this invocation's `agent_i` has index `idx_i`, then `results[idx_i]` yields `result_i`.
///
/// The difference between `bulk_then` and `default_bulk_then` is that unlike `default_bulk_then`, `bulk_then` is a customization point whose
/// behavior can be customized with a fancy execution policy.
///
/// \param policy An execution policy describing the requirements of the execution agents created by this call to `bulk_then`.
/// \param f      A function defining the work to be performed by execution agents.
/// \param predecessor A future object representing the predecessor task. Its future value, if it has one, is passed to `f` as an argument when `f` is invoked.
///                    After `bulk_then` returns, `predecessor` is invalid if it is not a shared future.
/// \param args   Additional arguments to pass to `f` when it is invoked.
/// \return `void`, if `f` has no result; otherwise, a container of `f`'s results indexed by the execution agent which produced them.
///
/// \tparam ExecutionPolicy This type must fulfill the requirements of `ExecutionPolicy`.
/// \tparam Function `Function`'s first parameter type must be `ExecutionPolicy::execution_agent_type&`.
///         The types of its additional parameters must match `Args...`.
/// \tparam Future This type must fulfill the requirements of `Future`. If the value type of this `Future` is not `void`, this type
///         must match the type of the second parameter of `Function`.
/// \tparam Args Each type in `Args...` must match the type of the corresponding parameter of `Function`.
///
/// The following example demonstrates how to use `bulk_then` to sequence a continuation after a predecessor task:
///
/// \include hello_then.cpp
///
/// Messages from agents in the predecessor task are guaranteed to be output before messages from the continuation:
///
/// ~~~~
/// $ clang -std=c++11 -I. -lstdc++ -pthread examples/hello_then.cpp -o hello_then
/// $ ./hello_then
/// Starting predecessor and continuation tasks asynchronously...
/// Sleeping before waiting on the continuation...
/// Hello, world from agent 0 in the predecessor task
/// Hello, world from agent 1 in the predecessor task
/// Hello, world from agent 2 in the predecessor task
/// Hello, world from agent 3 in the predecessor task
/// Hello, world from agent 4 in the predecessor task
/// Hello, world from agent 0 in the continuation
/// Hello, world from agent 1 in the continuation
/// Hello, world from agent 2 in the continuation
/// Hello, world from agent 3 in the continuation
/// Hello, world from agent 4 in the continuation
/// Woke up, waiting for the continuation to complete...
/// OK
/// ~~~~
///
/// \see default_bulk_then
/// \see bulk_invoke
/// \see bulk_async

#ifdef DOXYGEN_ONLY
template<class ExecutionPolicy, class Function, class... Args>
__AGENCY_ANNOTATION
see_below bulk_then(ExecutionPolicy&& policy, Function f, Args&&... args);
#else
namespace
{

#ifndef __CUDA_ARCH__
constexpr auto const& bulk_then = detail::static_const<detail::bulk_then_t>::value;
#else
// __device__ functions cannot access global variables, so make bulk_then a __device__ variable in __device__ code
const __device__ detail::bulk_then_t bulk_then;
#endif

} // end anonymous namespace
#endif


} // end agency

