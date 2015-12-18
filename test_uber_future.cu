#include <cassert>
#include "uber_future.hpp"
#include <iostream>

struct continuation_1
{
  __host__ __device__
  int operator()(int& arg)
  {
    return arg + 7;
  }
};

struct continuation_2
{
  __host__ __device__
  int operator()(int& arg)
  {
    return arg + 42;
  };
};

int main()
{
  static_assert(agency::detail::is_future<uber_future<int>>::value, "uber_future<int> is not a future");

  {
    // default construction
    uber_future<int> f0;
    assert(!f0.valid());
  }

  {
    // move construction
    uber_future<int> f0 = uber_future<int>::make_ready(13);
    assert(f0.valid());

    uber_future<int> f1 = std::move(f0);
    assert(!f0.valid());
    assert(f1.valid());
  }

  {
    // move assignment
    uber_future<int> f1 = uber_future<int>::make_ready(13);
    assert(f1.valid());

    uber_future<int> f2;
    assert(!f2.valid());

    f2 = std::move(f1);
    assert(!f1.valid());
    assert(f2.valid());
  }

  {
    // make_ready/then
    auto f1 = uber_future<int>::make_ready(13);

    assert(f1.valid());

    auto f2 = f1.then(continuation_1());

    assert(!f1.valid());
    assert(f2.valid());

    auto f3 = f2.then(continuation_2());

    assert(!f2.valid());
    assert(f3.valid());

    f3.wait();
    assert(f3.valid());

    assert(f3.get() == 13 + 7 + 42);

    assert(!f3.valid());
  }

  std::cout << "OK" << std::endl;

  return 0;
}

