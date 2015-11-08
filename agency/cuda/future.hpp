/*
 *  Copyright 2008-2013 NVIDIA Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#pragma once

#include <agency/detail/config.hpp>
#include <agency/cuda/detail/event.hpp>
#include <agency/cuda/detail/asynchronous_state.hpp>
#include <agency/cuda/detail/continuation.hpp>
#include <agency/future.hpp>
#include <agency/detail/type_traits.hpp>
#include <agency/detail/tuple.hpp>
#include <agency/detail/pointer.hpp>
#include <utility>
#include <type_traits>


namespace agency
{
namespace cuda
{
namespace detail
{


template<class T, class... Args>
struct is_constructible_or_void
  : std::integral_constant<
      bool,
      std::is_constructible<T,Args...>::value ||
      (std::is_void<T>::value && (sizeof...(Args) == 0))
    >
{};


__host__ __device__
unit get_value_or_unit(asynchronous_state<void>&)
{
  return unit{};
}

template<class T>
__host__ __device__
T get_value_or_unit(asynchronous_state<T>& state)
{
  return state.get();
}

template<class T>
__host__ __device__
typename asynchronous_state<T>::pointer get_data_pointer(asynchronous_state<T>& state)
{
  return state.data();
}


template<class... Types>
__host__ __device__
static auto get_values_or_units(asynchronous_state<Types>&... states)
  -> decltype(
       agency::detail::make_tuple(get_value_or_unit(states)...)
     )
{
  return agency::detail::make_tuple(get_value_or_unit(states)...);
}

template<class... Types>
__host__ __device__
static auto get_data_pointers(asynchronous_state<Types>&... states)
  -> decltype(
       agency::detail::make_tuple(get_data_pointer(states)...)
     )
{
  return agency::detail::make_tuple(get_data_pointer(states)...);
}

struct get_values_or_units_from_tuple_functor
{
  template<class... Args>
  __host__ __device__
  auto operator()(Args&&... args) const
    -> decltype(
         get_values_or_units(std::forward<Args>(args)...)
       )
  {
    return get_values_or_units(std::forward<Args>(args)...);
  }
};


template<class... Types>
__host__ __device__
auto get_values_or_units_from_tuple(agency::detail::tuple<asynchronous_state<Types>...>& tuple)
  -> decltype(
       agency::detail::tuple_apply(get_values_or_units_from_tuple_functor{}, tuple)
     )
{
  return agency::detail::tuple_apply(get_values_or_units_from_tuple_functor{}, tuple);
}


struct get_data_pointers_from_tuple_functor
{
  template<class... Args>
  __host__ __device__
  auto operator()(Args&&... args) const -> decltype(get_data_pointers(std::forward<Args>(args)...))
  {
    return get_data_pointers(std::forward<Args>(args)...);
  }
};


template<class... Types>
__host__ __device__
auto get_data_pointers_from_tuple(agency::detail::tuple<asynchronous_state<Types>...>& tuple)
  -> decltype(
       agency::detail::tuple_apply(get_data_pointers_from_tuple_functor{}, tuple)
     )
{
  return agency::detail::tuple_apply(get_data_pointers_from_tuple_functor{}, tuple);
}


template<class... Types>
class asynchronous_state_tuple : private agency::detail::tuple<asynchronous_state<Types>...>
{
  private:
    using super_t = agency::detail::tuple<asynchronous_state<Types>...>;

    __host__ __device__
    super_t& state_tuple()
    {
      return *this;
    }

    __host__ __device__
    const super_t& state_tuple() const
    {
      return *this;
    }

    __host__ __device__
    auto get_values_or_units_from_state_tuple()
      -> decltype(
           get_values_or_units_from_tuple(state_tuple())
         )
    {
      return get_values_or_units_from_tuple(state_tuple());
    }
    
    template<class T>
    struct is_not_unit : std::integral_constant<bool, !std::is_same<T,unit>::value> {};
    
    template<class Tuple>
    __host__ __device__
    static auto filter_non_unit(Tuple&& tuple)
      -> decltype(
           agency::detail::tuple_filter<is_not_unit>(std::forward<Tuple>(tuple))
         )
    {
      return agency::detail::tuple_filter<is_not_unit>(std::forward<Tuple>(tuple));
    }

    struct call_valid
    {
      template<class T>
      __AGENCY_ANNOTATION
      bool operator()(const asynchronous_state<T>& state) const
      {
        return state.valid();
      }
    };

  public:
    __host__ __device__
    asynchronous_state_tuple(asynchronous_state<Types>&&... states)
      : super_t(std::move(states)...)
    {}

    __host__ __device__
    auto get()
      -> decltype(
           agency::detail::unwrap_small_tuple(
             filter_non_unit(
               this->get_values_or_units_from_state_tuple()
             )
           )
         )
    {
      return agency::detail::unwrap_small_tuple(
        filter_non_unit(
          get_values_or_units_from_state_tuple()
        )
      );
    }

    using pointer = agency::detail::zip_pointer<typename asynchronous_state<Types>::pointer...>;

    __host__ __device__
    pointer data()
    {
      return get_data_pointers_from_tuple(state_tuple());
    }

    __host__ __device__
    bool valid() const
    {
      return agency::detail::tuple_all_of(state_tuple(), call_valid{});
    }
};


// declare this so future may befriend it
template<class Shape, class Index, class ThisIndexFunction>
class basic_grid_executor;

template<class U>
using element_type_is_not_unit = std::integral_constant<
  bool,
  !std::is_same<typename std::pointer_traits<U>::element_type, detail::unit>::value
>;


} // end detail


template<typename T>
class future
{
  private:
    cudaStream_t stream_;
    detail::event event_;
    detail::asynchronous_state<T> state_;

    // XXX stream_ should default to per-thread default stream
    static constexpr cudaStream_t default_stream{0};

  public:
    // XXX this should be private
    __host__ __device__
    future(cudaStream_t s) : future(s, detail::event{}) {}

    __host__ __device__
    future() : future(default_stream) {}

    __host__ __device__
    future(future&& other)
      : future()
    {
      future::swap(stream_, other.stream_);
      event_.swap(other.event_);
      state_.swap(other.state_);
    } // end future()

    __host__ __device__
    future &operator=(future&& other)
    {
      future::swap(stream_, other.stream_);
      future::swap(event_,  other.event_);
      future::swap(state_,  other.state_);
      return *this;
    } // end operator=()

    template<class U,
             class = typename std::enable_if<
               std::is_constructible<
                 detail::asynchronous_state<T>,
                 detail::asynchronous_state<U>&&
               >::value
             >::type>
    __host__ __device__
    future(future<U>&& other)
      : stream_(),
        event_(),
        state_(std::move(other.state_))
    {
      future::swap(stream_, other.stream_);
      event_.swap(other.event_);
    } // end future()

    __host__ __device__
    void wait() const
    {
      event_.wait();
    } // end wait()

    __host__ __device__
    T get()
    {
      wait();

      return state_.get();
    } // end get()

    __host__ __device__
    bool valid() const
    {
      return event_.valid() && state_.valid();
    } // end valid()

    __host__ __device__
    detail::event& event()
    {
      return event_;
    } // end event()

    __host__ __device__
    cudaStream_t stream() const
    {
      return stream_;
    } // end stream()

    template<class... Args,
             class = typename std::enable_if<
               detail::is_constructible_or_void<T,Args...>::value
             >::type>
    __host__ __device__
    static future make_ready(Args&&... args)
    {
      detail::event ready_event(detail::event::construct_ready);

      return future(std::move(ready_event), std::forward<Args>(args)...);
    }

    // XXX this is only used by grid_executor::then_execute()
    __host__ __device__
    auto data() -> decltype(state_.data())
    {
      return state_.data();
    }

    template<class Function>
    __host__ __device__
    future<agency::detail::result_of_continuation_t<Function,future>>
      then(Function f)
    {
      // create state for the continuation's result
      using result_type = agency::detail::result_of_continuation_t<Function,future>;
      detail::asynchronous_state<result_type> result_state(stream());

      // tuple up f's input state
      auto unfiltered_pointer_tuple = agency::detail::make_tuple(data());

      // filter void states
      auto pointer_tuple = agency::detail::tuple_filter<detail::element_type_is_not_unit>(unfiltered_pointer_tuple);

      // make a function implementing the continuation
      auto continuation = detail::make_continuation(f, result_state.data(), pointer_tuple);

      // launch the continuation
      detail::event next_event = event().then(continuation, dim3{1}, dim3{1}, 0, stream());

      // return the continuation's future
      return future<result_type>(stream(), std::move(next_event), std::move(result_state));
    }

//  private:
    template<class U> friend class future;
    template<class Shape, class Index, class ThisIndexFunction> friend class agency::cuda::detail::basic_grid_executor;

    __host__ __device__
    future(cudaStream_t s, detail::event&& e, detail::asynchronous_state<T>&& state)
      : stream_(s), event_(std::move(e)), state_(std::move(state))
    {}

    template<class... Args,
             class = typename std::enable_if<
               detail::is_constructible_or_void<T,Args...>::value
             >::type>
    __host__ __device__
    future(cudaStream_t s, detail::event&& e, Args&&... ready_args)
      : future(s, std::move(e), detail::asynchronous_state<T>(s, std::forward<Args>(ready_args)...))
    {}

    template<class... Args,
             class = typename std::enable_if<
               detail::is_constructible_or_void<T,Args...>::value
             >::type>
    __host__ __device__
    future(detail::event&& e, Args&&... ready_args)
      : future(default_stream, std::move(e), detail::asynchronous_state<T>(default_stream, std::forward<Args>(ready_args)...))
    {}

    // implement swap to avoid depending on thrust::swap
    template<class U>
    __host__ __device__
    static void swap(U& a, U& b)
    {
      U tmp{a};
      a = b;
      b = tmp;
    }

    template<class... Types>
    friend __host__ __device__
    future<
      agency::detail::when_all_result_t<
        future<Types>...
      >
    >
    when_all(future<Types>&... futures);
};


inline __host__ __device__
future<void> make_ready_future()
{
  return future<void>::make_ready();
} // end make_ready_future()

template<class T>
__host__ __device__
future<T> make_ready_future(T&& val)
{
  return future<T>::make_ready(std::forward<T>(val));
}


namespace detail
{


template<class Result>
struct when_all_functor
{
  template<class... Args>
  __host__ __device__
  Result operator()(Args&... args)
  {
    return Result(std::move(args)...);
  }
};


} // end detail


template<class... Types>
__host__ __device__
future<
  agency::detail::when_all_result_t<
    future<Types>...
  >
>
when_all(future<Types>&... futures)
{
  cudaStream_t stream = 0;

  // join the events
  detail::event when_all_ready = detail::when_all(stream, futures.event()...);

  using result_type = agency::detail::when_all_result_t<
    future<Types>...
  >;

  detail::asynchronous_state<result_type> result_state(stream);

  // tuple up the input states
  auto unfiltered_pointer_tuple = agency::detail::make_tuple(futures.data()...);

  // filter void states
  auto pointer_tuple = agency::detail::tuple_filter<detail::element_type_is_not_unit>(unfiltered_pointer_tuple);

  // make a function implementing the continuation
  auto continuation = detail::make_continuation(detail::when_all_functor<result_type>{}, result_state.data(), pointer_tuple);

  // launch the continuation
  detail::event next_event = when_all_ready.then(continuation, dim3{1}, dim3{1}, 0, stream);

  // return the continuation's future
  return future<result_type>(stream, std::move(next_event), std::move(result_state));
}


template<class T>
__host__ __device__
future<T> when_all(future<T>& future)
{
  return std::move(future);
}


} // end namespace cuda
} // end namespace agency

