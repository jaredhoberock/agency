// Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <agency/detail/config.hpp>
#include <type_traits>
#include <iostream>
#include <cassert>
#include <stdexcept>
#include <string>
#include <cstdio>


namespace agency
{
namespace detail
{
namespace variant_detail
{


template<size_t i, size_t... js>
struct constexpr_max
{
  static const size_t value = i < constexpr_max<js...>::value ? constexpr_max<js...>::value : i;
};


template<size_t i>
struct constexpr_max<i>
{
  static const size_t value = i;
};


} // end variant_detail


template<typename... Types>
class variant;


namespace variant_detail
{

template<size_t i, typename Variant> struct variant_element;


template<size_t i, typename T0, typename... Types>
struct variant_element<i, variant<T0, Types...>>
  : variant_element<i-1,variant<Types...>>
{};


template<typename T0, typename... Types>
struct variant_element<0, variant<T0, Types...>>
{
  typedef T0 type;
};


template<size_t i, typename... Types>
using variant_element_t = typename variant_element<i,Types...>::type;


} // end variant_detail


static constexpr const size_t variant_not_found = static_cast<size_t>(-1);


namespace variant_detail
{


template<typename T, typename U>
struct propagate_reference;


template<typename T, typename U>
struct propagate_reference<T&, U>
{
  typedef U& type;
};


template<typename T, typename U>
struct propagate_reference<const T&, U>
{
  typedef const U& type;
};


template<typename T, typename U>
struct propagate_reference<T&&, U>
{
  typedef U&& type;
};


template<size_t i, typename VariantReference>
struct variant_element_reference
  : propagate_reference<
      VariantReference,
      variant_element_t<
        i,
        typename std::decay<VariantReference>::type
      >
    >
{};


template<size_t i, typename VariantReference>
using variant_element_reference_t = typename variant_element_reference<i,VariantReference>::type;


} // end variant_detail


template<typename Visitor, typename Variant>
__AGENCY_ANNOTATION
typename std::result_of<
  Visitor&(variant_detail::variant_element_reference_t<0,Variant&&>)
>::type
  visit(Visitor& visitor, Variant&& var);


template<typename Visitor, typename Variant>
__AGENCY_ANNOTATION
typename std::result_of<
  const Visitor&(variant_detail::variant_element_reference_t<0,Variant&&>)
>::type
  visit(const Visitor& visitor, Variant&& var);


template<typename Visitor, typename Variant1, typename Variant2>
__AGENCY_ANNOTATION
typename std::result_of<
  Visitor&(variant_detail::variant_element_reference_t<0,Variant1&&>,
           variant_detail::variant_element_reference_t<0,Variant2&&>)
>::type
  visit(Visitor& visitor, Variant1&& var1, Variant2&& var2);


template<typename Visitor, typename Variant1, typename Variant2>
__AGENCY_ANNOTATION
typename std::result_of<
  const Visitor&(variant_detail::variant_element_reference_t<0,Variant1&&>,
                 variant_detail::variant_element_reference_t<0,Variant2&&>)
>::type
  visit(const Visitor& visitor, Variant1&& var1, Variant2&& var2);


namespace variant_detail
{


template<size_t i, typename T, typename... Types>
struct find_type_impl;


// no match, keep going
template<size_t i, typename T, typename U, typename... Types>
struct find_type_impl<i,T,U,Types...>
  : find_type_impl<i+1,T,Types...>
{};


// found a match
template<size_t i, typename T, typename... Types>
struct find_type_impl<i,T,T,Types...>
{
  static constexpr const size_t value = i;
};


// no match
template<size_t i, typename T>
struct find_type_impl<i,T>
{
  static constexpr const size_t value = variant_not_found;
};


template<typename T, typename... Types>
using find_type = find_type_impl<0,T,Types...>;


template<typename T, typename Variant>
struct is_variant_alternative;

template<class T, class... Types>
struct is_variant_alternative<T,variant<Types...>>
  : std::integral_constant<
      bool,
      (find_type<T,Types...>::value != variant_not_found)
    >
{};


} // end variant_detail


class bad_variant_access : public std::logic_error
{
  public:
    explicit bad_variant_access(const std::string& what_arg) : logic_error(what_arg) {}
    explicit bad_variant_access(const char* what_arg) : logic_error(what_arg) {}
};


namespace variant_detail
{


__AGENCY_ANNOTATION
inline void throw_bad_variant_access(const char* what_arg)
{
#ifdef __CUDA_ARCH__
  printf("bad_variant_access: %s\n", what_arg);
  assert(0);
#else
  throw bad_variant_access(what_arg);
#endif
}


template<class T, class...>
struct first_type
{
  using type = T;
};

template<class... Types>
using first_type_t = typename first_type<Types...>::type;


template<class... Types>
class variant_storage
{
  typedef typename std::aligned_storage<
    constexpr_max<sizeof(Types)...>::value
  >::type storage_type;
  
  storage_type storage_;
};

template<>
class variant_storage<> {};

} // end variant_detail


template<typename... Types>
class variant : private variant_detail::variant_storage<Types...>
{
  public:
    __AGENCY_ANNOTATION
    variant()
      : variant(variant_detail::first_type_t<Types...>{})
    {}

  private:
    struct binary_move_construct_visitor
    {
      template<class T>
      __AGENCY_ANNOTATION
      void operator()(T& self, T& other)
      {
        new (&self) T(std::move(other));
      }

      template<class... Args>
      __AGENCY_ANNOTATION
      void operator()(Args&&...){}
    };

  public:
    __AGENCY_ANNOTATION
    variant(variant&& other)
      : index_(other.index())
    {
      auto visitor = binary_move_construct_visitor();
      agency::detail::visit(visitor, *this, other);
    }

  private:
    struct binary_copy_construct_visitor
    {
      template<class T>
      __AGENCY_ANNOTATION
      void operator()(T& self, const T& other)
      {
        new (&self) T(other);
      }

      template<class... Args>
      __AGENCY_ANNOTATION
      void operator()(Args&&...){}
    };

  public:
    __AGENCY_ANNOTATION
    variant(const variant& other)
      : index_(other.index())
    {
      auto visitor = binary_copy_construct_visitor();
      agency::detail::visit(visitor, *this, other);
    }

  private:
    template<class T>
    struct unary_copy_construct_visitor
    {
      const T& other;

      __AGENCY_ANNOTATION
      void operator()(T& self)
      {
        new (&self) T(other);
      }

      template<class U>
      __AGENCY_ANNOTATION
      void operator()(U&&) {}
    };

  public:
    template<class T,
             class = typename std::enable_if<
               variant_detail::is_variant_alternative<T,variant>::value
             >::type>
    __AGENCY_ANNOTATION
    variant(const T& other)
      : index_(variant_detail::find_type<T,Types...>::value)
    {
      agency::detail::visit(unary_copy_construct_visitor<T>{other}, *this);
    }

  private:
    template<class T>
    struct unary_move_construct_visitor
    {
      T& other;

      __AGENCY_ANNOTATION
      void operator()(T& self)
      {
        new (&self) T(std::move(other));
      }

      template<class U>
      __AGENCY_ANNOTATION
      void operator()(U&&){}
    };

  public:
    template<class T,
             class = typename std::enable_if<
               variant_detail::is_variant_alternative<T,variant>::value
             >::type>
    __AGENCY_ANNOTATION
    variant(T&& other)
      : index_(variant_detail::find_type<T,Types...>::value)
    {
      auto visitor = unary_move_construct_visitor<T>{other};
      agency::detail::visit(visitor, *this);
    }

  private:
    struct destruct_visitor
    {
      template<typename T>
      __AGENCY_ANNOTATION
      typename std::enable_if<
        !std::is_trivially_destructible<T>::value
      >::type
        operator()(T& x)
      {
        x.~T();
      }
      
      template<typename T>
      __AGENCY_ANNOTATION
      typename std::enable_if<
        std::is_trivially_destructible<T>::value
      >::type
        operator()(T& x)
      {
        // omit invocations of destructors for trivially destructible types
      }
    };

  public:
    __AGENCY_ANNOTATION
    ~variant()
    {
      auto visitor = destruct_visitor();
      agency::detail::visit(visitor, *this);
    }

  private:
    struct copy_assign_visitor
    {
      template<class T>
      __AGENCY_ANNOTATION
      void operator()(T& self, const T& other) const
      {
        self = other;
      }

      template<class... Args>
      __AGENCY_ANNOTATION
      void operator()(Args&&...) const {}
    };

    struct destroy_and_copy_construct_visitor
    {
      template<class A, class B>
      __AGENCY_ANNOTATION
      void operator()(A& a, const B& b) const
      {
        // copy b to a temporary
        B tmp = b;

        // destroy a
        a.~A();

        // placement move from tmp to a
        new (&a) B(std::move(tmp));
      }
    };

    struct destroy_and_move_construct_visitor
    {
      template<class A, class B>
      __AGENCY_ANNOTATION
      void operator()(A& a, B&& b) const
      {
        // destroy a
        a.~A();

        using type = typename std::decay<B>::type;

        // placement move from b
        new (&a) type(std::move(b));
      }
    };

  public:
    __AGENCY_ANNOTATION
    variant& operator=(const variant& other)
    {
      if(index() == other.index())
      {
        agency::detail::visit(copy_assign_visitor(), *this, other);
      }
      else
      {
        agency::detail::visit(destroy_and_copy_construct_visitor(), *this, other);
        index_ = other.index();
      }

      return *this;
    }

  private:
    struct move_assign_visitor
    {
      template<class T>
      __AGENCY_ANNOTATION
      void operator()(T& self, T& other)
      {
        self = std::move(other);
      }

      template<class... Args>
      __AGENCY_ANNOTATION
      void operator()(Args&&...){}
    };


  public:
    __AGENCY_ANNOTATION
    variant& operator=(variant&& other)
    {
      if(index() == other.index())
      {
        auto visitor = move_assign_visitor();
        agency::detail::visit(visitor, *this, other);
      }
      else
      {
        auto visitor = destroy_and_move_construct_visitor();
        agency::detail::visit(visitor, *this, std::move(other));
        index_ = other.index();
      }

      return *this;
    }

    __AGENCY_ANNOTATION
    size_t index() const
    {
      return index_;
    }

  private:
    struct swap_visitor
    {
      template<class A, class B>
      __AGENCY_ANNOTATION
      void operator()(A& a, B& b)
      {
        // XXX can't call std::swap because __AGENCY_ANNOTATION
        A tmp = std::move(a);
        a = std::move(b);
        b = std::move(tmp);
      }
    };

  public:
    __AGENCY_ANNOTATION
    void swap(variant& other)
    {
      if(index() == other.index())
      {
        auto visitor = swap_visitor();
        agency::detail::visit(visitor, *this, other);
      }
      else
      {
        variant tmp = *this;
        *this = other;
        other = std::move(tmp);
      }
    }

  private:
    struct equals
    {
      template<typename U1, typename U2>
      __AGENCY_ANNOTATION
      bool operator()(const U1&, const U2&)
      {
        return false;
      }

      template<typename T>
      __AGENCY_ANNOTATION
      bool operator()(const T& lhs, const T& rhs)
      {
        return lhs == rhs;
      }
    };


  public:
    __AGENCY_ANNOTATION
    bool operator==(const variant& rhs) const
    {
      auto visitor = equals();
      return index() == rhs.index() && agency::detail::visit(visitor, *this, rhs);
    }

    __AGENCY_ANNOTATION
    bool operator!=(const variant& rhs) const
    {
      return !operator==(rhs);
    }

  private:
    struct less
    {
      template<typename U1, typename U2>
      __AGENCY_ANNOTATION
      bool operator()(const U1&, const U2&)
      {
        return false;
      }

      template<typename T>
      __AGENCY_ANNOTATION
      bool operator()(const T& lhs, const T& rhs)
      {
        return lhs < rhs;
      }
    };

  public:
    __AGENCY_ANNOTATION
    bool operator<(const variant& rhs) const
    {
      if(index() != rhs.index()) return index() < rhs.index();

      return agency::detail::visit(less(), *this, rhs);
    }

    __AGENCY_ANNOTATION
    bool operator<=(const variant& rhs) const
    {
      return !(rhs < *this);
    }

    __AGENCY_ANNOTATION
    bool operator>(const variant& rhs) const
    {
      return rhs < *this;
    }

    __AGENCY_ANNOTATION
    bool operator>=(const variant& rhs) const
    {
      return !(*this < rhs);
    }

  private:
    size_t index_;
};


namespace variant_detail
{


struct ostream_output_visitor
{
  std::ostream &os;

  ostream_output_visitor(std::ostream& os) : os(os) {}

  template<typename T>
  std::ostream& operator()(const T& x)
  {
    return os << x;
  }
};


} // variant_detail


template<typename... Types>
std::ostream &operator<<(std::ostream& os, const variant<Types...>& v)
{
  auto visitor = variant_detail::ostream_output_visitor(os);
  return agency::detail::visit(visitor, v);
}


namespace variant_detail
{


template<typename VisitorReference, typename Result, typename T, typename... Types>
struct apply_visitor_impl : apply_visitor_impl<VisitorReference,Result,Types...>
{
  typedef apply_visitor_impl<VisitorReference,Result,Types...> super_t;

  __agency_exec_check_disable__
  __AGENCY_ANNOTATION
  static Result do_it(VisitorReference visitor, void* ptr, size_t index)
  {
    if(index == 0)
    {
      return visitor(*reinterpret_cast<T*>(ptr));
    }

    return super_t::do_it(visitor, ptr, --index);
  }


  __agency_exec_check_disable__
  __AGENCY_ANNOTATION
  static Result do_it(VisitorReference visitor, const void* ptr, size_t index)
  {
    if(index == 0)
    {
      return visitor(*reinterpret_cast<const T*>(ptr));
    }

    return super_t::do_it(visitor, ptr, --index);
  }
};


template<typename VisitorReference, typename Result, typename T>
struct apply_visitor_impl<VisitorReference,Result,T>
{
  __agency_exec_check_disable__
  __AGENCY_ANNOTATION
  static Result do_it(VisitorReference visitor, void* ptr, size_t)
  {
    return visitor(*reinterpret_cast<T*>(ptr));
  }

  __agency_exec_check_disable__
  __AGENCY_ANNOTATION
  static Result do_it(VisitorReference visitor, const void* ptr, size_t)
  {
    return visitor(*reinterpret_cast<const T*>(ptr));
  }
};


template<typename VisitorReference, typename Result, typename Variant>
struct apply_visitor;


template<typename VisitorReference, typename Result, typename... Types>
struct apply_visitor<VisitorReference,Result,variant<Types...>>
  : apply_visitor_impl<VisitorReference,Result,Types...>
{};


} // end variant_detail


template<typename Visitor, typename Variant>
__AGENCY_ANNOTATION
typename std::result_of<
  Visitor&(variant_detail::variant_element_reference_t<0,Variant&&>)
>::type
  visit(Visitor& visitor, Variant&& var)
{
  using result_type = typename std::result_of<
    Visitor&(variant_detail::variant_element_reference_t<0,Variant&&>)
  >::type;

  using impl = variant_detail::apply_visitor<Visitor&,result_type,typename std::decay<Variant>::type>;

  return impl::do_it(visitor, &var, var.index());
}


template<typename Visitor, typename Variant>
__AGENCY_ANNOTATION
typename std::result_of<
  const Visitor&(variant_detail::variant_element_reference_t<0,Variant&&>)
>::type
  visit(const Visitor& visitor, Variant&& var)
{
  using result_type = typename std::result_of<
    const Visitor&(variant_detail::variant_element_reference_t<0,Variant&&>)
  >::type;
 
  using impl = variant_detail::apply_visitor<const Visitor&,result_type,typename std::decay<Variant>::type>;

  return impl::do_it(visitor, &var, var.index());
}


namespace variant_detail
{


template<typename VisitorReference, typename Result, typename ElementReference>
struct unary_visitor_binder
{
  VisitorReference visitor;
  ElementReference x;

  __AGENCY_ANNOTATION
  unary_visitor_binder(VisitorReference visitor, ElementReference x) : visitor(visitor), x(x) {}

  template<typename T>
  __AGENCY_ANNOTATION
  Result operator()(T&& y)
  {
    return visitor(x, std::forward<T>(y));
  }
};


template<class Reference>
struct rvalue_reference_to_lvalue_reference
{
  using type = Reference;
};

template<class T>
struct rvalue_reference_to_lvalue_reference<T&&>
{
  using type = T&;
};


template<typename VisitorReference, typename Result, typename VariantReference>
struct binary_visitor_binder
{
  VisitorReference visitor;
  // since rvalue references can't be members of classes, we transform any
  // VariantReference which is an rvalue reference to an lvalue reference
  // when we use y in operator(), we cast it back to the original reference type
  typename rvalue_reference_to_lvalue_reference<VariantReference>::type y;

  __AGENCY_ANNOTATION
  binary_visitor_binder(VisitorReference visitor, VariantReference ref) : visitor(visitor), y(ref) {}

  template<typename T>
  __AGENCY_ANNOTATION
  Result operator()(T&& x)
  {
    auto unary_visitor = unary_visitor_binder<VisitorReference, Result, decltype(x)>(visitor, std::forward<T>(x));
    return agency::detail::visit(unary_visitor, std::forward<VariantReference>(y));
  }
};


} // end variant_detail


template<typename Visitor, typename Variant1, typename Variant2>
__AGENCY_ANNOTATION
typename std::result_of<
  Visitor&(variant_detail::variant_element_reference_t<0,Variant1&&>,
           variant_detail::variant_element_reference_t<0,Variant2&&>)
>::type
  visit(Visitor& visitor, Variant1&& var1, Variant2&& var2)
{
  using result_type = typename std::result_of<
    Visitor&(variant_detail::variant_element_reference_t<0,Variant1&&>,
             variant_detail::variant_element_reference_t<0,Variant2&&>)
  >::type;

  auto visitor_wrapper = variant_detail::binary_visitor_binder<Visitor&,result_type,decltype(var2)>(visitor, std::forward<Variant2>(var2));

  return agency::detail::visit(visitor_wrapper, std::forward<Variant1>(var1));
}


template<typename Visitor, typename Variant1, typename Variant2>
__AGENCY_ANNOTATION
typename std::result_of<
  const Visitor&(variant_detail::variant_element_reference_t<0,Variant1&&>,
                 variant_detail::variant_element_reference_t<0,Variant2&&>)
>::type
  visit(const Visitor& visitor, Variant1&& var1, Variant2&& var2)
{
  using result_type = typename std::result_of<
    const Visitor&(variant_detail::variant_element_reference_t<0,Variant1&&>,
                   variant_detail::variant_element_reference_t<0,Variant2&&>)
  >::type;

  auto visitor_wrapper = variant_detail::binary_visitor_binder<const Visitor&,result_type,decltype(var2)>(visitor, std::forward<Variant2>(var2));

  return agency::detail::visit(visitor_wrapper, std::forward<Variant1>(var1));
}


namespace variant_detail
{


template<class T>
struct get_visitor
{
  __AGENCY_ANNOTATION
  T* operator()(T& x) const
  {
    return &x;
  }

  __AGENCY_ANNOTATION
  const T* operator()(const T& x) const
  {
    return &x;
  }

  template<class U>
  __AGENCY_ANNOTATION
  T* operator()(U&&) const
  {
    return nullptr;
  }
};

} // end variant_detail


template<size_t i, class... Types>
__AGENCY_ANNOTATION
variant_detail::variant_element_reference_t<i, variant<Types...>&>
  get(variant<Types...>& v)
{
  if(i != v.index())
  {
    variant_detail::throw_bad_variant_access("i does not equal index()");
  }

  using type = typename std::decay<
    variant_detail::variant_element_t<i,variant<Types...>>
  >::type;

  auto visitor = variant_detail::get_visitor<type>();
  return *agency::detail::visit(visitor, v);
}


template<size_t i, class... Types>
__AGENCY_ANNOTATION
variant_detail::variant_element_reference_t<i, variant<Types...>&&>
  get(variant<Types...>&& v)
{
  if(i != v.index())
  {
    variant_detail::throw_bad_variant_access("i does not equal index()");
  }

  using type = typename std::decay<
    variant_detail::variant_element_t<i,variant<Types...>>
  >::type;

  auto visitor = variant_detail::get_visitor<type>();
  return std::move(*agency::detail::visit(visitor, v));
}


template<size_t i, class... Types>
__AGENCY_ANNOTATION
variant_detail::variant_element_reference_t<i, const variant<Types...>&>
  get(const variant<Types...>& v)
{
  if(i != v.index())
  {
    variant_detail::throw_bad_variant_access("i does not equal index()");
  }

  using type = typename std::decay<
    variant_detail::variant_element_t<i,variant<Types...>>
  >::type;

  auto visitor = variant_detail::get_visitor<type>();
  return *agency::detail::visit(visitor, v);
}


namespace variant_detail
{


template<size_t i, class... Types>
struct find_exactly_one_impl;

template<size_t i, class T, class U, class... Types>
struct find_exactly_one_impl<i,T,U,Types...> : find_exactly_one_impl<i+1,T,Types...> {};

template<size_t i, class T, class... Types>
struct find_exactly_one_impl<i,T,T,Types...> : std::integral_constant<size_t, i>
{
  static_assert(find_exactly_one_impl<i,T,Types...>::value == variant_not_found, "type can only occur once in type list");
};


template<size_t i, class T>
struct find_exactly_one_impl<i,T> : std::integral_constant<size_t, variant_not_found> {};

template<class T, class... Types>
struct find_exactly_one : find_exactly_one_impl<0,T,Types...>
{
  static_assert(find_exactly_one::value != variant_not_found, "type not found in type list");
};


} // end variant_detail


template<class T, class... Types>
__AGENCY_ANNOTATION
bool holds_alternative(const variant<Types...>& v)
{
  constexpr size_t i = variant_detail::find_exactly_one<T,Types...>::value;
  return i == v.index();
}


template<class T, class... Types>
__AGENCY_ANNOTATION
typename std::remove_reference<T>::type&
  get(variant<Types...>& v)
{
  return get<variant_detail::find_type<T,Types...>::value>(v);
}


template<class T, class... Types>
__AGENCY_ANNOTATION
const typename std::remove_reference<T>::type&
  get(const variant<Types...>& v)
{
  return get<variant_detail::find_type<T,Types...>::value>(v);
}


template<class T, class... Types>
__AGENCY_ANNOTATION
typename std::remove_reference<T>::type&&
  get(variant<Types...>&& v)
{
  return std::move(get<variant_detail::find_type<T,Types...>::value>(v));
}


} // end detail
} // end agency

