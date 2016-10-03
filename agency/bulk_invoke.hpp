/// \file
/// \brief Include this file to use bulk_invoke().
///

#pragma once

#include <agency/detail/config.hpp>
#include <agency/detail/control_structures/bulk_invoke_execution_policy.hpp>
#include <agency/detail/type_traits.hpp>
#include <agency/detail/integer_sequence.hpp>
#include <agency/detail/control_structures/is_bulk_call_possible_via_execution_policy.hpp>
#include <agency/execution/execution_agent.hpp>
#include <agency/detail/requires.hpp>
#include <agency/detail/callable_parameter.hpp>

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

///
/// \defgroup control_structures Control Structures
/// \brief Control structures create execution.
///
///
/// The primary way Agency programs create execution is by invoking a
/// **control structure**. Control structures are functions invoked via
/// composition with an **execution policy**. Execution policies
/// parameterize control structures by describing the properties of the
/// requested execution.
///
/// For example, the following code snipped uses the bulk_invoke() control
/// structure with the \ref par execution policy to require the parallel execution
/// of ten invocations of a lambda function:
///
/// ~~~~{.cpp}
/// using namespace agency;
///
/// bulk_invoke(par(10), [](parallel_agent& self)
/// {
///   // task body here
///   ...
/// });
/// ~~~~


/// \brief Creates a bulk synchronous invocation.
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
/// \see bulk_async
/// \see bulk_then
template<class ExecutionPolicy, class Function, class... Args>
#ifndef DOXYGEN_SHOULD_SKIP_THIS
typename detail::enable_if_bulk_invoke_execution_policy<
  ExecutionPolicy, Function, Args...
>::type
#else
see_below
#endif
  bulk_invoke(ExecutionPolicy&& policy, Function f, Args&&... args)
{
  using agent_traits = execution_agent_traits<typename std::decay<ExecutionPolicy>::type::execution_agent_type>;
  const size_t num_shared_params = detail::execution_depth<typename agent_traits::execution_category>::value;

  return detail::bulk_invoke_execution_policy(detail::index_sequence_for<Args...>(), detail::make_index_sequence<num_shared_params>(), policy, f, std::forward<Args>(args)...);
}


namespace detail
{


// this is the implementation of the simple form of bulk_invoke()
template<class ExecutionPolicy, class Function, class... SharedParameterTypes>
auto simple_bulk_invoke_impl(ExecutionPolicy& policy, Function f, type_list<SharedParameterTypes...>) ->
  decltype(
    agency::bulk_invoke(
      policy, f,
      agency::share_at_scope<
        agency::detail::execution_policy_execution_depth<ExecutionPolicy>::value - 1,
        SharedParameterTypes
      >()...
    )
  )
{
  constexpr size_t innermost_scope = agency::detail::execution_policy_execution_depth<ExecutionPolicy>::value - 1;

  // workaround unused variable warning which can occur when SharedParameterTypes... is empty
  (void)(innermost_scope);

  return agency::bulk_invoke(policy, f,
                             agency::share_at_scope<innermost_scope, SharedParameterTypes>()...);
}


template<std::size_t I, class Function>
struct callable_parameter_is_default_and_move_constructible
  : std::integral_constant<
      bool,
      std::is_default_constructible<callable_parameter_or_t<I,Function,int>>::value &&
      std::is_move_constructible<callable_parameter_or_t<I,Function,int>>::value
    >
{};


// this function returns true if ExecutionPolicy and Function
// fulfill the requirements of the simple form of bulk_invoke()
template<class ExecutionPolicy, class Function>
constexpr bool simple_bulk_invoke_requirements()
{
  using policy_type = typename std::decay<ExecutionPolicy>::type;
  using agent_type = detail::execution_policy_agent_or_t<policy_type,void>;

  return is_callable<Function>::value
      && callable_parameter_is_same<
        0,
        Function,
        agent_type
      >::value
      // XXX we can relax move constructibility requirement in C++17, see wg21.link/p0135
      && callable_parameter_is_default_and_move_constructible<1,Function>::value 
      && callable_parameter_is_default_and_move_constructible<2,Function>::value 
      && callable_parameter_is_default_and_move_constructible<3,Function>::value 
      && callable_parameter_is_default_and_move_constructible<4,Function>::value 
      && callable_parameter_is_default_and_move_constructible<5,Function>::value 
      && callable_parameter_is_default_and_move_constructible<6,Function>::value 
      && callable_parameter_is_default_and_move_constructible<7,Function>::value 
      && callable_parameter_is_default_and_move_constructible<8,Function>::value 
      && callable_parameter_is_default_and_move_constructible<9,Function>::value 
      && callable_parameter_is_default_and_move_constructible<10,Function>::value
  ;
}

template<class ExecutionPolicy, class Function>
constexpr bool simple_bulk_invoke_success()
{
  using policy_type = typename std::decay<ExecutionPolicy>::type;
  using agent_type = detail::execution_policy_agent_or_t<policy_type,void>;

  return !is_call_possible<Function, agent_type&>::value && simple_bulk_invoke_requirements<ExecutionPolicy,Function>();
}

template<class ExecutionPolicy, class Function>
constexpr bool simple_bulk_invoke_error()
{
  using policy_type = typename std::decay<ExecutionPolicy>::type;
  using agent_type = detail::execution_policy_agent_or_t<policy_type,void>;

  return !is_call_possible<Function, agent_type&>::value && !simple_bulk_invoke_requirements<ExecutionPolicy,Function>();
}


} // end detail


// this is the normal path of simple bulk_invoke()
// XXX document this
// XXX we want to enable this overload when Function is not callable with the execution agent used as a single argument
//     AND when all the other simple bulk_invoke() requirements ARE fulfilled
template<class ExecutionPolicy, class Function,
         __AGENCY_REQUIRES(detail::simple_bulk_invoke_success<ExecutionPolicy,Function>())
        >
auto bulk_invoke(ExecutionPolicy&& policy, Function f) ->
  decltype(
    simple_bulk_invoke_impl(
      policy,
      f,
      detail::type_list_tail<detail::callable_parameter_list_t<Function>>()
    )
  )
{
  using function_parameter_types = detail::callable_parameter_list_t<Function>;

  // drop the first parameter -- it's the execution agent
  using shared_parameter_types = detail::type_list_tail<function_parameter_types>;

  return simple_bulk_invoke_impl(policy, f, shared_parameter_types());
}


// this is the error path of simple bulk_invoke(), its job is to output
// an error message indicating why the invocation is ill-formed
// XXX we want to enable this overload when Function is not callable with the execution agent used as a single argument
//     AND when any of the other simple bulk_invoke() requirements ARE NOT fulfilled
template<class ExecutionPolicy, class Function,
         __AGENCY_REQUIRES(detail::simple_bulk_invoke_error<ExecutionPolicy,Function>())
        >
void bulk_invoke(ExecutionPolicy&& policy, Function f)
{
  using policy_type = typename std::decay<ExecutionPolicy>::type;
  using agent_type = detail::execution_policy_agent_or_t<policy_type,void>;

  static_assert(detail::is_callable<Function>::value, "bulk_invoke(): Function must be either a function pointer or a class type with a single non-template operator().");

  using namespace agency::detail;

  using first_parameter_type = callable_parameter_or_t<0, Function, agent_type>;

  static_assert(std::is_same<agent_type, first_parameter_type>::value, "bulk_invoke(): Function's first parameter type must be the same as execution policy's execution_agent_type.");

  static_assert(std::is_default_constructible<callable_parameter_or_t<1,Function,int>>::value, "bulk_invoke(): Function's second parameter type must be default constructible.");
  static_assert(std::is_move_constructible<callable_parameter_or_t<1,Function,int>>::value, "bulk_invoke(): Function's second parameter type must be move constructible.");

  static_assert(std::is_default_constructible<callable_parameter_or_t<2,Function,int>>::value, "bulk_invoke(): Function's third parameter type must be default constructible.");
  static_assert(std::is_move_constructible<callable_parameter_or_t<2,Function,int>>::value, "bulk_invoke(): Function's third parameter type must be move constructible.");

  static_assert(std::is_default_constructible<callable_parameter_or_t<3,Function,int>>::value, "bulk_invoke(): Function's fourth parameter type must be default constructible.");
  static_assert(std::is_move_constructible<callable_parameter_or_t<3,Function,int>>::value, "bulk_invoke(): Function's fourth parameter type must be move constructible.");

  static_assert(std::is_default_constructible<callable_parameter_or_t<4,Function,int>>::value, "bulk_invoke(): Function's fifth parameter type must be default constructible.");
  static_assert(std::is_move_constructible<callable_parameter_or_t<4,Function,int>>::value, "bulk_invoke(): Function's fifth parameter type must be move constructible.");

  static_assert(std::is_default_constructible<callable_parameter_or_t<5,Function,int>>::value, "bulk_invoke(): Function's sixth parameter type must be default constructible.");
  static_assert(std::is_move_constructible<callable_parameter_or_t<5,Function,int>>::value, "bulk_invoke(): Function's sixth parameter type must be move constructible.");

  static_assert(std::is_default_constructible<callable_parameter_or_t<6,Function,int>>::value, "bulk_invoke(): Function's seventh parameter type must be default constructible.");
  static_assert(std::is_move_constructible<callable_parameter_or_t<6,Function,int>>::value, "bulk_invoke(): Function's seventh parameter type must be move constructible.");

  static_assert(std::is_default_constructible<callable_parameter_or_t<7,Function,int>>::value, "bulk_invoke(): Function's eigth parameter type must be default constructible.");
  static_assert(std::is_move_constructible<callable_parameter_or_t<7,Function,int>>::value, "bulk_invoke(): Function's eigth parameter type must be move constructible.");

  static_assert(std::is_default_constructible<callable_parameter_or_t<8,Function,int>>::value, "bulk_invoke(): Function's ninth parameter type must be default constructible.");
  static_assert(std::is_move_constructible<callable_parameter_or_t<8,Function,int>>::value, "bulk_invoke(): Function's ninth parameter type must be move constructible.");

  static_assert(std::is_default_constructible<callable_parameter_or_t<9,Function,int>>::value, "bulk_invoke(): Function's tenth parameter type must be default constructible.");
  static_assert(std::is_move_constructible<callable_parameter_or_t<9,Function,int>>::value, "bulk_invoke(): Function's tenth parameter type must be move constructible.");
  
  static_assert(std::is_default_constructible<callable_parameter_or_t<10,Function,int>>::value, "bulk_invoke(): Function's eleventh parameter type must be default constructible.");
  static_assert(std::is_move_constructible<callable_parameter_or_t<10,Function,int>>::value, "bulk_invoke(): Function's eleventh parameter type must be move constructible.");
}


} // end agency

