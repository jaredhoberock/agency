/******************************************************************************
 * Copyright (C) 2016-2017, Cris Cecka.  All rights reserved.
 * Copyright (C) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the NVIDIA CORPORATION nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#pragma once

#include <agency/detail/config.hpp>
#include <agency/functional/detail/multi_function.hpp>
#include <agency/functional/invoke.hpp>
#include <agency/detail/type_traits.hpp>
#include <utility>
#include <type_traits>


// Inspired by
// * Eric Niebler's suggested design for customization points and
// * Cris Cecka's design for customizing customization points
// * Jared Hoberock's design for multi_functions and customization points

namespace agency
{
namespace detail
{


// this function converts a call to a customization point object:
//
//     customization_point(customizer, args...)
//
// into a call to agency::invoke:
//
//     agency::invoke(customizer, customization_point, args...)
//
// The customizer is retained as the first parameter,
// but the customization point object itself is passed as the second parameter,
// followed by the rest of the arguments to the customization_point call.
struct invoke_customization_point
{
  template<class CustomizationPoint, class Customizer, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(CustomizationPoint&& self, Customizer&& customizer, Args&&... args) const ->
    decltype(agency::invoke(std::forward<Customizer>(customizer), std::forward<CustomizationPoint>(self), std::forward<Args>(args)...))
  {
    return agency::invoke(std::forward<Customizer>(customizer), std::forward<CustomizationPoint>(self), std::forward<Args>(args)...);
  }
};


// this functor wraps another Function
// when drop_first_arg_and_invoke is called,
// it ignores its first argument and calls the Function with the remaining arguments
template<class Function>
struct drop_first_arg_and_invoke
{
  Function f;

  __agency_exec_check_disable__
  template<class Arg1, class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(Arg1&&, Args&&... args) const ->
    decltype(f(std::forward<Args>(args)...))
  {
    return f(std::forward<Args>(args)...);
  }
};


// customization_point is a class for creating Niebler-style customization points
// which attempt one implementation after another, in order, to find an appropriate
// dispatch.
//
// A customization point differs from a multi_function only in that it passes itself
// as the first parameter to each of the potential dispatch functions. Users may use
// detail::drop_first_arg_and_invoke for a simpler interface.
//
// When a customization_point is called like a function:
//
//    (*this)(arg1, args...);
//
// it tries each possible function it was created over, in order:
//
// function1(DerivedOrCP, args...)
// function2(DerivedOrCP, args...)
// ...
//
// The first implementation that is not ill-formed is called.
// If all of these implementations are ill-formed, then the call
// to the customization_point is ill-formed.

template<class Derived, class... Functions>
class customization_point : multi_function<Functions...>
{
 private:
  using super_t = multi_function<Functions...>;

  // Derived is allowed to be void, which indicates there is no
  // inheritor of customization_point
  using derived_type = conditional_t<
    std::is_void<Derived>::value,
    customization_point,
    Derived
  >;

  __AGENCY_ANNOTATION
  const derived_type& self() const
  {
    return static_cast<const derived_type&>(*this);
  }

  __AGENCY_ANNOTATION
  const super_t& super() const
  {
    return static_cast<const super_t&>(*this);
  }

 public:
  constexpr customization_point() = default;

  __AGENCY_ANNOTATION
  constexpr customization_point(Functions... funcs)
      : super_t(funcs...)
  {}

  template <class... Args>
  __AGENCY_ANNOTATION
  constexpr auto operator()(Args&&... args) const ->
      decltype(super()(self(), std::forward<Args>(args)...))
  {
    // note how we call the derived-from multi_function like a function,
    // but we insert ourself as the first parameter
    return super()(self(), std::forward<Args>(args)...);
  }
};


template<class... Functions, class Derived = void>
__AGENCY_ANNOTATION
constexpr customization_point<Derived,Functions...>
make_customization_point(Functions... funcs)
{
  return customization_point<Derived,Functions...>(funcs...);
}


} // end namespace detail
} // end namespace agency

