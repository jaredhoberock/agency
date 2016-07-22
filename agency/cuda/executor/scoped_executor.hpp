#pragma once

// XXX seems like this should not be a public header and instead be automatically
//     included by grid_executor.hpp, parallel_executor.hpp, and block_executor.hpp
//     to be sure we catch the specialization
#include <agency/cuda/executor/grid_executor.hpp>
#include <agency/cuda/executor/parallel_executor.hpp>
#include <agency/cuda/executor/block_executor.hpp>

namespace agency
{


template<>
class scoped_executor<cuda::parallel_executor, cuda::block_executor>
  : public cuda::grid_executor
{
  public:
    // XXX might be worth adapting the underlying cuda::grid_executor in some way that would
    //     yield outer & inner executor types
    using outer_executor_type = cuda::parallel_executor;
    using inner_executor_type = cuda::block_executor;

    scoped_executor() = default;

    scoped_executor(const outer_executor_type& outer_ex,
                    const inner_executor_type& inner_ex)
      : outer_ex_(outer_ex),
        inner_ex_(inner_ex)
    {}

    outer_executor_type& outer_executor()
    {
      return outer_ex_;
    }

    const outer_executor_type& outer_executor() const
    {
      return outer_ex_;
    }

    inner_executor_type& inner_executor()
    {
      return inner_ex_;
    }

    const inner_executor_type& inner_executor() const
    {
      return inner_ex_;
    }

  private:
    outer_executor_type outer_ex_;
    inner_executor_type inner_ex_;
}; // end scoped_executor


} // end agency

