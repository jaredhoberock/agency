#include <agency/bulk_then.hpp>
#include <agency/execution_policy.hpp>
#include <iostream>

template<class ExecutionPolicy>
void test()
{
  using execution_policy_type = ExecutionPolicy;
  using executor_type = typename ExecutionPolicy::executor_type;

  {
    // bulk_then with non-void future and no parameters
    
    execution_policy_type policy;

    auto fut = agency::executor_traits<executor_type>::template make_ready_future<int>(policy.executor(), 7);

    auto f = agency::bulk_then(policy(10),
      [](typename execution_policy_type::execution_agent_type& self, int& return_me)
      {
        return return_me;
      },
      fut
    );

    auto result = f.get();

    using container_type = typename agency::executor_traits<executor_type>::template container<int>;

    assert(container_type(10,7) == result);
  }

  {
    // bulk_then with void future and no parameters
    
    execution_policy_type policy;

    auto fut = agency::executor_traits<executor_type>::template make_ready_future<void>(policy.executor());

    auto f = agency::bulk_then(policy(10),
      [](typename execution_policy_type::execution_agent_type& self)
      {
        return 7;
      },
      fut
    );

    auto result = f.get();

    using container_type = typename agency::executor_traits<executor_type>::template container<int>;

    assert(container_type(10,7) == result);
  }

  {
    // bulk_then with non-void future and one parameter
    
    execution_policy_type policy;

    auto fut = agency::executor_traits<executor_type>::template make_ready_future<int>(policy.executor(), 7);

    int val = 13;

    auto f = agency::bulk_then(policy(10),
      [](typename execution_policy_type::execution_agent_type& self, int& past_arg, int val)
    {
      return past_arg + val;
    },
    fut,
    val);

    auto result = f.get();

    using container_type = typename agency::executor_traits<executor_type>::template container<int>;

    assert(container_type(10,7 + 13) == result);
  }

  {
    // bulk_then with void future and one parameter
    
    execution_policy_type policy;

    auto fut = agency::executor_traits<executor_type>::template make_ready_future<void>(policy.executor());

    int val = 13;

    auto f = agency::bulk_then(policy(10),
      [](typename execution_policy_type::execution_agent_type& self, int val)
    {
      return val;
    },
    fut,
    val);

    auto result = f.get();

    using container_type = typename agency::executor_traits<executor_type>::template container<int>;

    assert(container_type(10,13) == result);
  }

  {
    // bulk_then with non-void future and one shared parameter
    
    execution_policy_type policy;

    auto fut = agency::executor_traits<executor_type>::template make_ready_future<int>(policy.executor(), 7);

    int val = 13;

    auto f = agency::bulk_then(policy(10),
      [](typename execution_policy_type::execution_agent_type& self, int& past_arg, int& shared_arg)
    {
      return past_arg + shared_arg;
    },
    fut,
    agency::share(val));

    auto result = f.get();

    using container_type = typename agency::executor_traits<executor_type>::template container<int>;

    assert(container_type(10,7 + 13) == result);
  }

  {
    // bulk_then with void future and one shared parameter
    
    execution_policy_type policy;

    auto fut = agency::executor_traits<executor_type>::template make_ready_future<void>(policy.executor());

    int val = 13;

    auto f = agency::bulk_then(policy(10),
      [](typename execution_policy_type::execution_agent_type& self, int& shared_arg)
    {
      return shared_arg;
    },
    fut,
    agency::share(val));

    auto result = f.get();

    using container_type = typename agency::executor_traits<executor_type>::template container<int>;

    assert(container_type(10,13) == result);
  }
}

int main()
{
  test<agency::sequential_execution_policy>();
  test<agency::concurrent_execution_policy>();
  test<agency::parallel_execution_policy>();

  std::cout << "OK" << std::endl;

  return 0;
}

