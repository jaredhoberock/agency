#pragma once

#include <agency/detail/config.hpp>
#include <agency/detail/requires.hpp>
#include <agency/detail/invoke.hpp>
#include <agency/detail/type_traits.hpp>
#include <agency/execution/executor/detail/utility/invoke_functors.hpp>
#include <agency/execution/executor/customization_points/bulk_sync_execute.hpp>
#include <type_traits>


namespace agency
{
namespace detail
{


template<class E, class Function, class ResultFactory, class... SharedFactories,
         __AGENCY_REQUIRES(BulkExecutor<E>()),
         __AGENCY_REQUIRES(executor_execution_depth<E>::value == sizeof...(SharedFactories)),
         __AGENCY_REQUIRES(!std::is_void<result_of_t<Function(executor_index_t<E>, result_of_t<SharedFactories()>&...)>>::value)
        >
__AGENCY_ANNOTATION
result_of_t<ResultFactory()>
  bulk_sync_execute_with_collected_result(E& exec, Function f, executor_shape_t<E> shape, ResultFactory result_factory, SharedFactories... shared_factories)
{
  // wrap f in a functor that will collect f's result and call bulk_sync_execute()
  return agency::bulk_sync_execute(exec, invoke_and_collect_result<Function>{f}, shape, result_factory, shared_factories...);
}


} // end detail
} // end agency

