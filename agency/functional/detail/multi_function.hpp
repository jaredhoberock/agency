#pragma once

#include <agency/detail/config.hpp>
#include <utility>

namespace agency
{
namespace detail
{


template<class... Implementations>
class multi_function;

template<>
class multi_function<> {};


// a multi_function has several different implementations
// when called like a function, the multi_function selects the first implementation that is not ill-formed 
template<class Implementation1, class... Implementations>
class multi_function<Implementation1, Implementations...> : multi_function<Implementations...>
{
  private:
    using super_t = multi_function<Implementations...>;

    mutable Implementation1 impl_;

    __agency_exec_check_disable__
    template<class... Args>
    __AGENCY_ANNOTATION
    static constexpr auto impl(const multi_function& self, Args&&... args) ->
      decltype(self.impl_(std::forward<Args>(args)...))
    {
      return self.impl_(std::forward<Args>(args)...);
    }

    __agency_exec_check_disable__
    template<class... Args>
    __AGENCY_ANNOTATION
    static constexpr auto impl(const super_t& super, Args&&... args) ->
      decltype(super(std::forward<Args>(args)...))
    {
      return super(std::forward<Args>(args)...); 
    }

  public:
    constexpr multi_function() = default;

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    constexpr multi_function(Implementation1 impl1, Implementations... impls)
      : multi_function<Implementations...>(impls...), impl_(impl1)
    {}

    __agency_exec_check_disable__
    template<class... Args>
    __AGENCY_ANNOTATION
    constexpr auto operator()(Args&&... args) const ->
      decltype(multi_function::impl(*this, std::forward<Args>(args)...))
    {
      return multi_function::impl(*this, std::forward<Args>(args)...);
    }
};


} // end detail
} // end agency

