#include <iostream>
#include <agency/bulk_invoke.hpp>
#include <agency/cuda/execution_policy.hpp>

struct functor
{
  __device__
  void operator()(agency::concurrent_agent& self)
  {
    printf("agent %d arriving at barrier\n", (int)self.index());

    self.wait();

    printf("departing barrier\n");
  }
};

int main()
{
  agency::cuda::concurrent_executor gpu;
  
  agency::bulk_invoke(agency::con(10).on(gpu), functor());

  return 0;
}

