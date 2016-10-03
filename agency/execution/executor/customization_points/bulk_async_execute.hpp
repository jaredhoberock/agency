#pragma once

#include <agency/detail/config.hpp>
#include <agency/detail/requires.hpp>
#include <agency/future.hpp>
#include <agency/execution/executor/customization_points/bulk_then_execute.hpp>
#include <agency/execution/executor/executor_traits.hpp>
#include <agency/detail/type_traits.hpp>

namespace agency
{


__agency_exec_check_disable__
template<class E, class Function, class ResultFactory, class... Factories,
         __AGENCY_REQUIRES(detail::BulkAsynchronousExecutor<E>()),
         __AGENCY_REQUIRES(executor_execution_depth<E>::value == sizeof...(Factories))
        >
__AGENCY_ANNOTATION
executor_future_t<
  E,
  detail::result_of_t<ResultFactory()>
>
bulk_async_execute(E& exec, Function f, executor_shape_t<E> shape, ResultFactory result_factory, Factories... shared_factories)
{
  return exec.bulk_async_execute(f, shape, result_factory, shared_factories...);
}


__agency_exec_check_disable__
template<class E, class Function, class ResultFactory, class... Factories,
         __AGENCY_REQUIRES(detail::BulkExecutor<E>() && !detail::BulkAsynchronousExecutor<E>()),
         __AGENCY_REQUIRES(executor_execution_depth<E>::value == sizeof...(Factories))
        >
__AGENCY_ANNOTATION
executor_future_t<
  E,
  detail::result_of_t<ResultFactory()>
>
bulk_async_execute(E& exec, Function f, executor_shape_t<E> shape, ResultFactory result_factory, Factories... shared_factories)
{
  using void_future_type = executor_future_t<E,void>;

  // XXX we might want to actually allow the executor to participate here
  auto predecessor = future_traits<void_future_type>::make_ready();

  return agency::bulk_then_execute(exec, f, shape, predecessor, result_factory, shared_factories...);
}


} // end agency

