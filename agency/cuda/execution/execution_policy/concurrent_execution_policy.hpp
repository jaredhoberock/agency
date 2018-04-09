#pragma once

#include <agency/detail/config.hpp>
#include <agency/execution/execution_policy/basic_execution_policy.hpp>
#include <agency/cuda/execution/executor/concurrent_executor.hpp>

namespace agency
{
namespace cuda
{


class concurrent_execution_policy : public basic_execution_policy<concurrent_agent, cuda::concurrent_executor, concurrent_execution_policy>
{
  private:
    using super_t = basic_execution_policy<concurrent_agent, cuda::concurrent_executor, concurrent_execution_policy>;

  public:
    using super_t::basic_execution_policy;
};


const concurrent_execution_policy con{};


class concurrent_execution_policy_2d : public basic_execution_policy<concurrent_agent_2d, cuda::concurrent_executor, concurrent_execution_policy_2d>
{
  private:
    using super_t = basic_execution_policy<concurrent_agent_2d, cuda::concurrent_executor, concurrent_execution_policy_2d>;

  public:
    using super_t::basic_execution_policy;
};


const concurrent_execution_policy_2d con2d{};


} // end cuda
} // end agency

