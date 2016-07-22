#pragma once

#include <agency/detail/config.hpp>
#include <agency/executor/detail/this_thread_parallel_executor.hpp>
#include <agency/detail/concurrency/thread_pool.hpp>

namespace agency
{


using parallel_executor = detail::parallel_thread_pool_executor;


} // end agency

