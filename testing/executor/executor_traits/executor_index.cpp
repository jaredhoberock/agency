#include <agency/execution/executor/executor_traits.hpp>
#include <type_traits>
#include <iostream>

struct not_an_executor {};

struct bulk_executor_without_index_type
{
  template<class Function, class ResultFactory, class SharedFactory>
  typename std::result_of<ResultFactory()>::type
  bulk_sync_execute(Function f, size_t n, ResultFactory result_factory, SharedFactory shared_factory);
};

struct bulk_executor_with_shape_type_without_index_type
{
  struct shape_type
  {
    size_t n;
  };

  template<class Function, class ResultFactory, class SharedFactory>
  typename std::result_of<ResultFactory()>::type
  bulk_sync_execute(Function f, shape_type n, ResultFactory result_factory, SharedFactory shared_factory);
};

struct bulk_executor_with_index_type
{
  struct index_type
  {
    size_t i;
  };

  template<class Function, class ResultFactory, class SharedFactory>
  typename std::result_of<ResultFactory()>::type
  bulk_sync_execute(Function f, size_t n, ResultFactory result_factory, SharedFactory shared_factory);
};

struct bulk_executor_with_shape_type_with_index_type
{
  struct shape_type
  {
    size_t n;
  };

  struct index_type
  {
    size_t i;
  };

  template<class Function, class ResultFactory, class SharedFactory>
  typename std::result_of<ResultFactory()>::type
  bulk_sync_execute(Function f, shape_type n, ResultFactory result_factory, SharedFactory shared_factory);
};

int main()
{
  static_assert(!agency::detail::is_detected<agency::executor_index_t, not_an_executor>::value,
    "executor_index_t<not_an_executor> should not be detected");

  static_assert(agency::detail::is_detected_exact<size_t, agency::executor_index_t, bulk_executor_without_index_type>::value,
    "bulk_executor_without_index_type should have size_t index_type");

  static_assert(agency::detail::is_detected_exact<bulk_executor_with_shape_type_without_index_type::shape_type, agency::executor_index_t, bulk_executor_with_shape_type_without_index_type>::value,
    "bulk_executor_with_shape_type_without_index_type should have bulk_executor_with_shape_type_without_index_type::shape_type index_type");

  static_assert(agency::detail::is_detected_exact<bulk_executor_with_index_type::index_type, agency::executor_index_t, bulk_executor_with_index_type>::value,
    "bulk_executor_with_index_type should have bulk_executor_with_index_type::index_type index_type");

  static_assert(agency::detail::is_detected_exact<bulk_executor_with_shape_type_with_index_type::index_type, agency::executor_index_t, bulk_executor_with_shape_type_with_index_type>::value,
    "bulk_executor_with_shape_type_with_index_type should have bulk_executor_with_shape_type_with_index_type::index_type index_type");

  std::cout << "OK" << std::endl;

  return 0;
}

