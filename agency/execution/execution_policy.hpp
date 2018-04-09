/// \file
/// \brief Contains definitions of built-in execution policies.
///

/// \defgroup execution_policies Execution Policies
/// \ingroup execution
/// \brief Execution policies describe requirements for execution.
///
/// Execution policies describe the execution properties of bulk tasks created by control structures such as `bulk_invoke()`.
/// Such properties include both *how* and *where* execution should occur. Forward progress requirements encapsulated by 
/// execution policies describe the ordering relationships of individual execution agents comprising a bulk task, while the execution policy's
/// associated *executor* governs where those execution agents execute.
///
/// ### Essential Characteristics
///
/// An execution policy collects two essential characteristics: a type of execution agent defining execution requirements,
/// and an associated executor which creates execution with prescribed guarantees. When combined with control structures
/// like bulk_invoke(), the associated executor creates execution and the characteristics of this execution are reified
/// in the program as execution agent objects.
///
/// ### Parameterization
///
/// Aside from these characteristics, execution policy objects also encapsulate a *parameterization* describing
/// the group of execution agents to create when composed with a control structure. For most of Agency's execution agent types,
/// these parameters define the range of indices assigned to agents in the group.
///
/// An existing instance of an execution policy may be called like a function to produce a different an instance with a
/// different parameterization. For example, the execution policy agency::par may be called like a function to create a new
/// policy with a different parameterization:
///
/// ~~~~{.cpp}
/// // call seq like a function to produce an execution policy generating 13 agents
/// agency::bulk_invoke(agency::seq(13), [](agency::sequenced_agent& self)
/// {
///   std::cout << self.index() << std::endl;
/// });
///
/// // the integers [0,13) are printed in sequence
/// ~~~~
///
/// Alternatively, we can shift the origin of the group by passing agency::seq a half-open range:
///
/// ~~~~{.cpp}
/// agency::bulk_invoke(agency::seq(10,23), [](agency::sequenced_agent& self)
/// {
///   std::cout << self.index() << std::endl;
/// });
///
/// // the integers [10,23) are printed in sequence
/// ~~~~
///
/// ### The associated executor
///
/// Each of Agency's execution policies have an associated executor. The member function `.executor()` provides access to this executor:
///
/// ~~~~{.cpp}
/// // make a copy of par's associated executor
/// agency::parallel_executor par_exec = agency::par.executor();
/// ~~~~
///
/// The type of an execution policy's associated executor is named by the member type `executor_type`. Generic contexts such as templates may use this type:
///
/// ~~~~{.cpp}
/// template<class ExecutionPolicy>
/// void foo(ExecutionPolicy& policy)
/// {
///   // use the member type executor_type to make a copy of policy's associated executor
///   typename ExecutionPolicy::executor_type exec1 = policy.executor();
///
///   // alternatively, use auto
///   auto exec2 = policy.executor();
///
///   ...
/// }
/// ~~~~
///
/// ### Replacing an executor with `.on()`
///
/// An existing execution policy's associated executor may be *replaced* with the `.on()` member function. `.on()`
/// creates a new execution policy object whose associated executor is a copy of the given executor:
///
/// ~~~~{.cpp}
/// // suppose I have some existing executor
/// agency::sequenced_executor my_executor;
///
/// // associate my_executor with a new policy derived from agency::par
/// auto new_policy = agency::par.on(my_executor);
///
/// // now all execution generated by new_policy will be created "on" my_executor
/// ~~~~

#pragma once

#include <utility>
#include <functional>
#include <type_traits>
#include <functional>
#include <memory>
#include <tuple>
#include <initializer_list>

#include <agency/execution/execution_policy/basic_execution_policy.hpp>
#include <agency/execution/execution_policy/concurrent_execution_policy.hpp>
#include <agency/execution/execution_policy/execution_policy_traits.hpp>
#include <agency/execution/execution_policy/parallel_execution_policy.hpp>
#include <agency/execution/execution_policy/sequenced_execution_policy.hpp>
#include <agency/execution/execution_policy/unsequenced_execution_policy.hpp>

// XXX the stuff defined down there should be moved into separate headers

namespace agency
{
namespace experimental
{
namespace detail
{


template<class ExecutionPolicy, std::size_t group_size,
         std::size_t grain_size = 1,
         class ExecutionAgent = basic_static_execution_agent<
           agency::detail::execution_policy_agent_t<ExecutionPolicy>,
           group_size,
           grain_size
         >,
         class Executor       = agency::detail::execution_policy_executor_t<ExecutionPolicy>>
using basic_static_execution_policy = basic_execution_policy<
  ExecutionAgent,
  Executor
>;


} // end detail


template<size_t group_size, size_t grain_size = 1>
class static_sequenced_execution_policy : public detail::basic_static_execution_policy<agency::sequenced_execution_policy, group_size, grain_size>
{
  private:
    using super_t = detail::basic_static_execution_policy<agency::sequenced_execution_policy, group_size, grain_size>;

  public:
    using super_t::super_t;
};


template<size_t group_size, size_t grain_size = 1>
class static_concurrent_execution_policy : public detail::basic_static_execution_policy<
  agency::concurrent_execution_policy,
  group_size,
  grain_size,
  static_concurrent_agent<group_size, grain_size>
>
{
  private:
    using super_t = detail::basic_static_execution_policy<
      agency::concurrent_execution_policy,
      group_size,
      grain_size,
      static_concurrent_agent<group_size, grain_size>
    >;

  public:
    using super_t::super_t;
};


} // end experimental
} // end agency

