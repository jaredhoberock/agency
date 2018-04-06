#pragma once

#include <agency/detail/config.hpp>
#include <agency/execution/execution_policy/basic_execution_policy.hpp>
#include <agency/cuda/execution/executor/parallel_executor.hpp>
#include <agency/cuda/execution/executor/grid_executor.hpp>
#include <agency/cuda/execution/executor/multidevice_executor.hpp>
#include <agency/cuda/device.hpp>


namespace agency
{
namespace cuda
{


class parallel_execution_policy : public basic_execution_policy<parallel_agent, cuda::parallel_executor, parallel_execution_policy>
{
  private:
    using super_t = basic_execution_policy<parallel_agent, cuda::parallel_executor, parallel_execution_policy>;

  public:
    using super_t::basic_execution_policy;
};


const parallel_execution_policy par{};


class parallel_execution_policy_2d : public basic_execution_policy<parallel_agent_2d, cuda::parallel_executor, parallel_execution_policy_2d>
{
  private:
    using super_t = basic_execution_policy<parallel_agent_2d, cuda::parallel_executor, parallel_execution_policy_2d>;

  public:
    using super_t::basic_execution_policy;
};


const parallel_execution_policy_2d par2d{};


// XXX this function needs to account for the dimensionality of ParallelPolicy's agents
template<class ParallelPolicy,
         __AGENCY_REQUIRES(
           agency::detail::policy_is_parallel<ParallelPolicy>::value
         )>
__AGENCY_ANNOTATION
cuda::parallel_execution_policy replace_executor(const ParallelPolicy& policy, const parallel_executor& exec)
{
  return cuda::parallel_execution_policy(policy.param(), exec);
}


// XXX this function needs to account for the dimensionality of ParallelPolicy's agents
template<class ParallelPolicy,
         __AGENCY_REQUIRES(
           agency::detail::policy_is_parallel<ParallelPolicy>::value
         )>
__AGENCY_ANNOTATION
cuda::parallel_execution_policy replace_executor(const ParallelPolicy& policy, const grid_executor& exec)
{
  // "flatten" the grid_executor into a parallel_executor
  cuda::parallel_executor parallel_exec(exec);

  // call a lower-level version of replace_executor()
  return cuda::replace_executor(policy, parallel_exec);
}


} // end cuda
} // end agency

