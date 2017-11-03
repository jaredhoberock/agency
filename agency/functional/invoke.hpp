#pragma once

#include <agency/detail/config.hpp>
#include <agency/detail/requires.hpp>
#include <agency/detail/control_structures/bind.hpp>
#include <agency/execution/executor/executor_traits.hpp>
#include <agency/execution/executor/customization_points/sync_execute.hpp>
#include <agency/functional/detail/multi_function.hpp>
#include <agency/detail/static_const.hpp>
#include <agency/detail/type_traits.hpp>
#include <utility>

namespace agency
{


// default_invoke takes as parameters either
// 1. an executor, a function, and an argument list or
// 2. a function, and an argument list


template<class Executor, class Function, class... Args,
         __AGENCY_REQUIRES(is_executor<Executor>::value)
        >
__AGENCY_ANNOTATION
detail::result_of_t<
  typename std::decay<Function&&>::type(typename std::decay<Args&&>::type...)
>
  default_invoke(Executor& exec, Function&& f, Args&&... args)
{
  auto g = detail::bind(std::forward<Function>(f), std::forward<Args>(args)...);

  return agency::sync_execute(exec, std::move(g));
}


__agency_exec_check_disable__
template<class Function, class... Args,
         __AGENCY_REQUIRES(!is_executor<typename std::decay<Function>::type>::value)
        >
__AGENCY_ANNOTATION
detail::result_of_t<
  typename std::decay<Function&&>::type(typename std::decay<Args&&>::type...)
>
  default_invoke(Function&& f, Args&&... args)
{
  return std::forward<Function>(f)(std::forward<Args>(args)...);
}


// the agency::invoke customization point is defined below
namespace detail
{


struct call_member_function_invoke
{
  __agency_exec_check_disable__
  template<class Customizer, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(Customizer&& customizer, Args&&... args) const ->
    decltype(std::forward<Customizer>(customizer).invoke(std::forward<Args>(args)...))
  {
    return std::forward<Customizer>(customizer).invoke(std::forward<Args>(args)...);
  }
};


struct call_free_function_invoke
{
  __agency_exec_check_disable__
  template<class Customizer, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(Customizer&& customizer, Args&&... args) const ->
    decltype(invoke(std::forward<Customizer>(customizer), std::forward<Args>(args)...))
  {
    return invoke(std::forward<Customizer>(customizer), std::forward<Args>(args)...);
  }
};


struct call_default_invoke
{
  template<class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(Args&&... args) const ->
    decltype(agency::default_invoke(std::forward<Args>(args)...))
  {
    return agency::default_invoke(std::forward<Args>(args)...);
  }
};


using invoke_t = multi_function<
  detail::call_member_function_invoke,
  detail::call_free_function_invoke,
  detail::call_default_invoke
>;


} // end detail


namespace
{


#ifndef __CUDA_ARCH__
constexpr auto const& invoke = detail::static_const<agency::detail::invoke_t>::value;
#else
// __device__ functions cannot access global variables, so make invoke a __device__ variable in __device__ code
const __device__ detail::invoke_t invoke;
#endif


} // end anonymous namespace


} // end agency

