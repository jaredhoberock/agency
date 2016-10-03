#pragma once

#include <agency/detail/config.hpp>
#include <agency/detail/type_traits.hpp>
#include <agency/execution/executor/executor_traits/is_bulk_executor.hpp>
#include <agency/execution/executor/executor_traits/detail/member_container_or.hpp>
#include <agency/execution/executor/executor_traits/executor_index.hpp>
#include <agency/execution/executor/executor_traits/executor_shape.hpp>
#include <agency/execution/executor/executor_traits/executor_allocator.hpp>
#include <agency/detail/array.hpp>


namespace agency
{
namespace detail
{


template<class BulkExecutor, class T, bool Enable = is_bulk_executor<BulkExecutor>::value>
struct executor_container_impl
{
};

template<class BulkExecutor, class T>
struct executor_container_impl<BulkExecutor,T,true>
{
  template<class U>
  using default_container = agency::detail::array<T, executor_shape_t<BulkExecutor>, executor_allocator_t<BulkExecutor,T>, executor_index_t<BulkExecutor>>;

  using type = member_container_or_t<BulkExecutor,T,default_container>;
};


} // end detail


template<class BulkExecutor, class T>
struct executor_container : detail::executor_container_impl<BulkExecutor,T> {};

template<class BulkExecutor, class T>
using executor_container_t = typename executor_container<BulkExecutor,T>::type;


} // end agency

