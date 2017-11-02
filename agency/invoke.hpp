#pragma once

#include <agency/detail/config.hpp>
#include <agency/detail/requires.hpp>
#include <agency/detail/control_structures/bind.hpp>
#include <agency/execution/executor/executor_traits.hpp>
#include <agency/execution/executor/customization_points/sync_execute.hpp>
#include <agency/detail/type_traits.hpp>
#include <utility>

namespace agency
{


template<class Executor, class Function, class... Args,
         __AGENCY_REQUIRES(is_executor<Executor>::value)
        >
__AGENCY_ANNOTATION
detail::result_of_t<
  typename std::decay<Function&&>::type(typename std::decay<Args&&>::type...)
>
  invoke(Executor& exec, Function&& f, Args&&... args)
{
  auto g = detail::bind(std::forward<Function>(f), std::forward<Args>(args)...);

  return agency::sync_execute(exec, std::move(g));
}


template<class Function, class... Args,
         __AGENCY_REQUIRES(!is_executor<typename std::decay<Function>::type>::value)
        >
detail::result_of_t<
  typename std::decay<Function&&>::type(typename std::decay<Args&&>::type...)
>
  invoke(Function&& f, Args&&... args)
{
  return std::forward<Function>(f)(std::forward<Args>(args)...);
}


} // end agency

