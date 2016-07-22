#include <agency/bulk_then.hpp>
#include <agency/cuda/execution_policy.hpp>
#include <iostream>

template<class T, class Function>
struct device_lambda_wrapper
{
  Function f;

  template<class... Args>
  __device__
  T operator()(Args&&... args)
  {
    return f(std::forward<Args>(args)...);
  }
};


template<class T, class Function>
device_lambda_wrapper<T,Function> wrap(Function f)
{
  return device_lambda_wrapper<T,Function>{f};
}

template<class ExecutionPolicy>
void test(ExecutionPolicy policy)
{
  using agent = typename ExecutionPolicy::execution_agent_type;
  using agent_traits = agency::execution_agent_traits<agent>;
  using executor_type = typename ExecutionPolicy::executor_type;

  {
    // non-void future and no parameters

    auto fut = agency::executor_traits<executor_type>::template make_ready_future<int>(policy.executor(), 7);

    auto f = agency::bulk_then(policy,
      wrap<int>([] __device__ (agent& self, int& return_me)
      {
        return return_me;
      }),
      fut
    );

    auto result = f.get();

    using container_type = typename agency::executor_traits<executor_type>::template container<int>;

    auto shape = agent_traits::domain(policy.param()).shape();

    using container_shape_t = typename container_type::shape_type;
    auto container_shape = agency::detail::shape_cast<container_shape_t>(shape);

    assert(container_type(container_shape,7) == result);
  }

  {
    // void future and no parameters

    auto fut = agency::executor_traits<executor_type>::template make_ready_future<void>(policy.executor());

    auto f = agency::bulk_then(policy,
      wrap<int>([] __device__ (agent& self)
      {
        return 7;
      }),
      fut
    );

    auto result = f.get();

    using container_type = typename agency::executor_traits<executor_type>::template container<int>;

    auto shape = agent_traits::domain(policy.param()).shape();

    using container_shape_t = typename container_type::shape_type;
    auto container_shape = agency::detail::shape_cast<container_shape_t>(shape);

    assert(container_type(container_shape,7) == result);
  }

  {
    // non-void future and one parameter

    auto fut = agency::executor_traits<executor_type>::template make_ready_future<int>(policy.executor(), 7);

    int val = 13;

    auto f = agency::bulk_then(policy,
      wrap<int>([] __device__ (agent& self, int& past_arg, int val)
      {
        return past_arg + val;
      }),
      fut,
      val
    );

    auto result = f.get();

    using container_type = typename agency::executor_traits<executor_type>::template container<int>;

    auto shape = agent_traits::domain(policy.param()).shape();

    using container_shape_t = typename container_type::shape_type;
    auto container_shape = agency::detail::shape_cast<container_shape_t>(shape);

    assert(container_type(container_shape,7 + 13) == result);
  }

  {
    // void future and one parameter

    auto fut = agency::executor_traits<executor_type>::template make_ready_future<void>(policy.executor());

    int val = 13;

    auto f = agency::bulk_then(policy,
      wrap<int>([] __device__ (agent& self, int val)
      {
        return val;
      }),
      fut,
      val
    );

    auto result = f.get();

    using container_type = typename agency::executor_traits<executor_type>::template container<int>;

    auto shape = agent_traits::domain(policy.param()).shape();

    using container_shape_t = typename container_type::shape_type;
    auto container_shape = agency::detail::shape_cast<container_shape_t>(shape);

    assert(container_type(container_shape,13) == result);
  }

  {
    // non-void future and one shared parameter

    auto fut = agency::executor_traits<executor_type>::template make_ready_future<int>(policy.executor(), 7);

    int val = 13;

    auto f = agency::bulk_then(policy,
      wrap<int>([] __device__ (agent& self, int& past_arg, int& shared_arg)
      {
        return past_arg + shared_arg;
      }),
      fut,
      agency::share(val)
    );

    auto result = f.get();

    using container_type = typename agency::executor_traits<executor_type>::template container<int>;

    auto shape = agent_traits::domain(policy.param()).shape();

    using container_shape_t = typename container_type::shape_type;
    auto container_shape = agency::detail::shape_cast<container_shape_t>(shape);

    assert(container_type(container_shape,7 + 13) == result);
  }

  {
    // void future and one shared parameter

    auto fut = agency::executor_traits<executor_type>::template make_ready_future<void>(policy.executor());

    int val = 13;

    auto f = agency::bulk_then(policy,
      wrap<int>([] __device__ (agent& self, int& shared_arg)
      {
        return shared_arg;
      }),
      fut,
      agency::share(val)
    );

    auto result = f.get();

    using container_type = typename agency::executor_traits<executor_type>::template container<int>;

    auto shape = agent_traits::domain(policy.param()).shape();

    using container_shape_t = typename container_type::shape_type;
    auto container_shape = agency::detail::shape_cast<container_shape_t>(shape);

    assert(container_type(container_shape,13) == result);
  }
}

int main()
{
  using namespace agency::cuda;

  test(par(10));
  test(par(10, con(10)));

  std::cout << "OK" << std::endl;

  return 0;
}

