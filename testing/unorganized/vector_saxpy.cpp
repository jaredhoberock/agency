#include <algorithm>
#include <cassert>
#include <iostream>
#include <agency/bulk_invoke.hpp>
#include <agency/execution_policy.hpp>

int main()
{
  using namespace agency;

  size_t n = 1 << 16;
  std::vector<float> x(n, 1), y(n, 2), z(n);
  float a = 13.;

  bulk_invoke(vec(n), [&](vector_agent &self)
  {
    auto i = self.index();
    z[i] = a * x[i] + y[i];
  });

  float expected  = a * 1. + 2.;
  assert(std::all_of(z.begin(), z.end(), [=](float x){ return expected == x; }));

  std::cout << "OK" << std::endl;

  return 0;
}

