#include <agency/bulk_async.hpp>
#include <agency/cuda/execution_policy.hpp>
#include <iostream>

template<class ExecutionPolicy>
void test(ExecutionPolicy policy)
{
  using agent = typename ExecutionPolicy::execution_agent_type;

  {
    // bulk_async with no parameters

    auto f = agency::bulk_async(policy,
      [] __host__ __device__ (agent& self) -> agency::single_result<int>
    {
      if(self.elect())
      {
        return 7;
      }

      return std::ignore;
    });

    auto result = f.get();

    assert(result == 7);
  }

  {
    // bulk_async with one parameter

    int val = 13;

    auto f = agency::bulk_async(policy,
      [] __host__ __device__ (agent& self, int val) -> agency::single_result<int>
    {
      if(self.elect())
      {
        return val;
      }

      return std::ignore;
    },
    val);

    auto result = f.get();

    assert(result == 13);
  }

  {
    // bulk_async with one shared parameter

    int val = 13;

    auto f = agency::bulk_async(policy,
      [] __host__ __device__ (agent& self, int& val) -> agency::single_result<int>
    {
      if(self.elect())
      {
        return val;
      }

      return std::ignore;
    },
    agency::share(val));

    auto result = f.get();

    assert(result == 13);
  }
}

int main()
{
  using namespace agency::cuda;

  test(par(10));
  test(con(10));

  test(par(10, con(10)));

  std::cout << "OK" << std::endl;

  return 0;
}

