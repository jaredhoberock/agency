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


// this is the general form of bulk_invoke()
// XXX use __AGENCY_REQUIRES()
// XXX provide an error path for better error messages
// XXX document this
template<class ExecutionPolicy, class Function, class... Args>
typename detail::enable_if_bulk_invoke_execution_policy<
  ExecutionPolicy, Function, Args...
>::type
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

