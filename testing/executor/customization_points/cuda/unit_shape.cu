#include <iostream>
#include <cassert>
#include <agency/execution/executor/customization_points/unit_shape.hpp>

#include "../../test_executors.hpp"

template<class Executor>
void test(Executor exec)
{
  using shape_type = agency::executor_shape_t<Executor>;

  shape_type unit = agency::unit_shape(exec);

  assert(unit == shape_type(1));
}

int main()
{
  test(bulk_synchronous_executor());
  test(bulk_asynchronous_executor());
  test(bulk_continuation_executor());

  test(not_a_bulk_synchronous_executor());
  test(not_a_bulk_asynchronous_executor());
  test(not_a_bulk_continuation_executor());

  test(complete_bulk_executor());

  std::cout << "OK" << std::endl;

  return 0;
}

