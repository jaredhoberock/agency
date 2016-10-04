#include <agency/agency.hpp>
#include <cassert>
#include <iostream>
#include <atomic>


template<int init>
class initialized_to
{
  public:
    initialized_to()
      : value_(init)
    {}

    int& value()
    {
      return value_;
    }

    const int& value() const
    {
      return value_;
    }

  private:
    int value_;
};


template<class ExecutionPolicy>
void test(ExecutionPolicy policy)
{
  {
    // bulk_invoke with one automatic shared parameter returning results

    auto result = agency::bulk_invoke(policy(10),
      [](typename ExecutionPolicy::execution_agent_type& self, initialized_to<13>& shared)
    {
      return shared.value();
    });

    using executor_type = typename ExecutionPolicy::executor_type;
    using container_type = agency::executor_container_t<executor_type,int>;

    assert(container_type(10,13) == result);
  }

  {
    // bulk_invoke with two automatic shared parameters returning results

    auto result = agency::bulk_invoke(policy(10),
      [](typename ExecutionPolicy::execution_agent_type& self, initialized_to<13>& shared1, initialized_to<7>& shared2)
    {
      return shared1.value() + shared2.value();
    });

    using executor_type = typename ExecutionPolicy::executor_type;
    using container_type = agency::executor_container_t<executor_type,int>;

    assert(container_type(10,13 + 7) == result);
  }

  {
    // bulk_invoke with one automatic shared parameter returning void

    std::atomic<int> increment_me(0);

    agency::bulk_invoke(policy(10),
      [&](typename ExecutionPolicy::execution_agent_type& self, initialized_to<13>& val)
    {
      increment_me += val.value();
    });

    assert(increment_me == 13 * 10);
  }

  {
    // bulk_invoke with two automatic shared parameters returning void

    std::atomic<int> increment_me(0);

    agency::bulk_invoke(policy(10),
      [&](typename ExecutionPolicy::execution_agent_type& self, initialized_to<13>& shared1, initialized_to<7>& shared2)
    {
      increment_me += shared1.value() + shared2.value();
    });

    assert(increment_me == (13 + 7) * 10);
  }
}


int main()
{
  test(agency::seq);
  test(agency::con);
  test(agency::par);
  test(agency::unseq);

  std::cout << "OK" << std::endl;

  return 0;
}

