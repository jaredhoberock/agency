#pragma once

#include <agency/detail/config.hpp>
#include <agency/detail/requires.hpp>
#include <agency/detail/default_shape.hpp>
#include <agency/experimental/ndarray/ndarray_ref.hpp>
#include <agency/memory/allocator/allocator.hpp>
#include <agency/detail/iterator/constant_iterator.hpp>
#include <agency/execution/execution_policy/detail/simple_sequenced_policy.hpp>
#include <agency/detail/algorithm/construct_n.hpp>
#include <agency/detail/algorithm/construct_array.hpp>
#include <agency/detail/algorithm/destroy.hpp>
#include <agency/detail/algorithm/destroy_array.hpp>
#include <agency/detail/algorithm/equal.hpp>
#include <agency/experimental/ndarray/constant_ndarray.hpp>
#include <utility>
#include <memory>
#include <iterator>


namespace agency
{
namespace experimental
{


template<class T, class Shape = size_t, class Alloc = agency::allocator<T>, class Index = Shape>
class basic_ndarray : private agency::detail::storage<T,Alloc,Shape,Index>
{
  private:
    using super_t = agency::detail::storage<T,Alloc,Shape,Index>;

  public:
    using value_type = T;
    using allocator_type = typename std::allocator_traits<Alloc>::template rebind_alloc<value_type>;
    using size_type = typename std::allocator_traits<allocator_type>::size_type;
    using pointer = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;

    using shape_type = typename super_t::shape_type;
    using index_type = Index;

    using iterator = pointer;
    using const_iterator = const_pointer;

    using reference = typename std::iterator_traits<iterator>::reference;
    using const_reference = typename std::iterator_traits<const_iterator>::reference;

    // note that basic_ndarray's constructors have __agency_exec_check_disable__
    // because Alloc's constructors may not have __AGENCY_ANNOTATION

    // XXX we should reformulate all the other constructors such that they lower onto this
    //     general purpose constructor
    __agency_exec_check_disable__
    template<class... Args, __AGENCY_REQUIRES(std::is_constructible<T, const Args&...>::value)>
    __AGENCY_ANNOTATION
    basic_ndarray(const shape_type& shape, const allocator_type& alloc, const Args&... constructor_args)
      : super_t(shape, alloc)
    {
      construct_elements_from_arrays(constant_ndarray<Args,Shape>(shape, constructor_args)...);
    }

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    basic_ndarray() : basic_ndarray(allocator_type()) {}

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    explicit basic_ndarray(const allocator_type& alloc) : basic_ndarray(shape_type{}, alloc) {}

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    explicit basic_ndarray(const shape_type& shape, const allocator_type& alloc = allocator_type())
      : super_t(shape, alloc)
    {
      construct_elements_from_arrays();
    }

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    basic_ndarray(const shape_type& shape, const T& val, const allocator_type& alloc = allocator_type())
      : basic_ndarray(constant_ndarray<T,Shape>(shape, val), alloc)
    {}

    __agency_exec_check_disable__
    template<class ArrayView,
             __AGENCY_REQUIRES(
               std::is_constructible<
                 super_t, decltype(std::declval<ArrayView>().shape()), allocator_type
               >::value
             )>
    __AGENCY_ANNOTATION
    explicit basic_ndarray(const ArrayView& array, const allocator_type& alloc = allocator_type())
      : super_t(array.shape(), alloc)
    {
      construct_elements_from_arrays(array);
    }

    template<class ExecutionPolicy,
             class Iterator,
             __AGENCY_REQUIRES(
               is_execution_policy<typename std::decay<ExecutionPolicy>::type>::value
             ),
             // XXX this requirement should really be something like is_input_iterator<InputIterator>
             __AGENCY_REQUIRES(
               std::is_convertible<typename std::iterator_traits<Iterator>::value_type, value_type>::value
             )>
    basic_ndarray(ExecutionPolicy&& policy, Iterator first, shape_type shape, const allocator_type& alloc = allocator_type())
      : super_t(shape, alloc)
    {
      construct_elements(std::forward<ExecutionPolicy>(policy), first);
    }

    template<class Iterator,
             // XXX this requirement should really be something like is_input_iterator<InputIterator>
             __AGENCY_REQUIRES(
               std::is_convertible<typename std::iterator_traits<Iterator>::value_type, value_type>::value
             )>
    basic_ndarray(Iterator first, shape_type shape, const allocator_type& alloc = allocator_type())
      : basic_ndarray(agency::detail::simple_sequenced_policy<>(), first, shape, alloc)
    {}

    __agency_exec_check_disable__
    template<class Iterator,
             // XXX this requirement should really be something like is_input_iterator<InputIterator>
             __AGENCY_REQUIRES(
               std::is_convertible<typename std::iterator_traits<Iterator>::value_type, value_type>::value
             )>
    __AGENCY_ANNOTATION
    basic_ndarray(Iterator first, Iterator last, const allocator_type& alloc = allocator_type())
      : basic_ndarray(first, agency::detail::shape_cast<shape_type>(last - first), alloc)
    {}

    __agency_exec_check_disable__
    template<class ExecutionPolicy,
             __AGENCY_REQUIRES(
               is_execution_policy<typename std::decay<ExecutionPolicy>::type>::value
             )>
    __AGENCY_ANNOTATION
    basic_ndarray(ExecutionPolicy&& policy, const basic_ndarray& other)
      : super_t(other.shape(), other.get_allocator())
    {
      construct_elements_from_arrays(std::forward<ExecutionPolicy>(policy), other.all());
    }

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    basic_ndarray(const basic_ndarray& other)
      : basic_ndarray(agency::detail::simple_sequenced_policy<index_type>(), other)
    {}

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    basic_ndarray(basic_ndarray&& other)
      : super_t{}
    {
      swap(other);
    }

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    ~basic_ndarray()
    {
      clear();
    }

    __AGENCY_ANNOTATION
    basic_ndarray& operator=(const basic_ndarray& other)
    {
      // XXX this is not a very efficient implementation
      basic_ndarray tmp = other;
      swap(tmp);
      return *this;
    }

    __AGENCY_ANNOTATION
    basic_ndarray& operator=(basic_ndarray&& other)
    {
      swap(other);
      return *this;
    }

    __AGENCY_ANNOTATION
    void swap(basic_ndarray& other)
    {
      super_t::swap(other);
    }


    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    allocator_type get_allocator() const
    {
      return super_t::allocator();
    }

    __AGENCY_ANNOTATION
    reference operator[](index_type idx)
    {
      return all()[idx];
    }

    __AGENCY_ANNOTATION
    const_reference operator[](index_type idx) const
    {
      return all()[idx];
    }

    __AGENCY_ANNOTATION
    shape_type shape() const
    {
      return super_t::shape();
    }

    __AGENCY_ANNOTATION
    std::size_t size() const
    {
      return super_t::size();
    }

    __AGENCY_ANNOTATION
    pointer data()
    {
      return super_t::data();
    }

    __AGENCY_ANNOTATION
    const_pointer data() const
    {
      return super_t::data();
    }

    __AGENCY_ANNOTATION
    basic_ndarray_ref<const T,shape_type,index_type,const_pointer> all() const
    {
      return super_t::all();
    }

    __AGENCY_ANNOTATION
    basic_ndarray_ref<T,shape_type,index_type,pointer> all()
    {
      return super_t::all();
    }

    __AGENCY_ANNOTATION
    iterator begin()
    {
      return all().data();
    }

    __AGENCY_ANNOTATION
    iterator end()
    {
      return all().end();
    }

    __AGENCY_ANNOTATION
    const_iterator begin() const
    {
      return all().begin();
    }

    __AGENCY_ANNOTATION
    const_iterator cbegin() const
    {
      return begin();
    }

    __AGENCY_ANNOTATION
    const_iterator end() const
    {
      return all().end();
    }

    __AGENCY_ANNOTATION
    const_iterator cend() const
    {
      return end();
    }

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    void clear()
    {
      agency::detail::destroy_array(super_t::allocator(), all());

      // empty the super class
      static_cast<super_t&>(*this) = super_t(std::move(super_t::allocator()));
    }

    __agency_exec_check_disable__
    template<class Range>
    __AGENCY_ANNOTATION
    friend bool operator==(const basic_ndarray& lhs, const Range& rhs)
    {
      return lhs.size() == rhs.size() && agency::detail::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    __agency_exec_check_disable__
    template<class Range>
    __AGENCY_ANNOTATION
    friend bool operator==(const Range& lhs, const basic_ndarray& rhs)
    {
      return rhs == lhs;
    }

    // this operator== avoids ambiguities introduced by the template friends above
    __AGENCY_ANNOTATION
    bool operator==(const basic_ndarray& rhs) const
    {
      return size() == rhs.size() && agency::detail::equal(begin(), end(), rhs.begin());
    }

  private:
    template<class ExecutionPolicy, class... Iterators,
             __AGENCY_REQUIRES(
               is_execution_policy<typename std::decay<ExecutionPolicy>::type>::value
             )>
    __AGENCY_ANNOTATION
    void construct_elements(ExecutionPolicy&& policy, Iterators... iters)
    {
      agency::detail::construct_n(std::forward<ExecutionPolicy>(policy), super_t::allocator(), begin(), size(), iters...);
    }

    template<class... Iterators>
    __AGENCY_ANNOTATION
    void construct_elements(Iterators... iters)
    {
      agency::detail::simple_sequenced_policy<> seq;
      construct_elements(seq, iters...);
    }


    template<class ExecutionPolicy, class... ArrayViews,
             __AGENCY_REQUIRES(
               is_execution_policy<typename std::decay<ExecutionPolicy>::type>::value
             )>
    __AGENCY_ANNOTATION
    void construct_elements_from_arrays(ExecutionPolicy&& policy, const ArrayViews&... arrays)
    {
      agency::detail::construct_array(super_t::allocator(), std::forward<ExecutionPolicy>(policy), all(), arrays...);
    }

    template<class... ArrayViews>
    __AGENCY_ANNOTATION
    void construct_elements_from_arrays(const ArrayViews&... arrays)
    {
      agency::detail::construct_array(super_t::allocator(), all(), arrays...);
    }
};


template<class T, size_t rank, class Alloc = agency::allocator<T>>
using ndarray = basic_ndarray<T, agency::detail::default_shape_t<rank>, Alloc>;


} // end experimental
} // end agency

