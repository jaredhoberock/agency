#pragma once

#include <agency/detail/config.hpp>
#include <agency/execution/execution_policy/basic_execution_policy.hpp>
#include <agency/cuda/execution/executor/concurrent_grid_executor.hpp>
#include <agency/cuda/execution/executor/scoped_executor.hpp>
#include <agency/cuda/device.hpp>

namespace agency
{
namespace cuda
{


// XXX consider making this a global object like the other execution policies
inline auto con_grid(size_t num_blocks, size_t num_threads) ->
  decltype(
    con(num_blocks, con(num_threads))
  )
{
  return con(num_blocks, con(num_threads));
};


// XXX consider making this a unique type instead of an alias
using con_grid_agent = concurrent_group<concurrent_agent>;


} // end cuda
} // end agency

