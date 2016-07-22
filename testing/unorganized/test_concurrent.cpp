#include <iostream>
#include <agency/bulk_async.hpp>
#include <agency/execution_policy.hpp>

int main()
{
  using namespace agency;

  bulk_async(con(10), [](concurrent_agent &g)
  {
    std::cout << "agent " << g.index() << " arriving at barrier" << std::endl;

    g.wait();

    std::cout << "departing barrier" << std::endl;
  }).wait();

  return 0;
}

