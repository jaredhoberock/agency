#pragma once

#include <agency/detail/config.hpp>
#include <agency/memory/allocator.hpp>
#include <agency/memory/allocator/detail/allocator_traits.hpp>
#include <agency/experimental/ndarray/ndarray_ref.hpp>


namespace agency
{
namespace detail
{


// storage takes an optional Shape parameter instead of assuming size_t
// so that multidimensional containers need not store their shape in
// addition to what is maintained by storage
//
// storage inherits from basic_ndarray_ref to enable zero-cost, nested std::span-like types from containers
// XXX template parameter T is redundant with Allocator
template<class T, class Allocator, class Shape = std::size_t, class Index = Shape>
class storage : private agency::experimental::basic_ndarray_ref<typename std::allocator_traits<Allocator>::pointer, Shape, Index>
{
  public:
    using allocator_type = Allocator;
    using value_type = typename std::allocator_traits<allocator_type>::value_type;
    using pointer = typename std::allocator_traits<allocator_type>::pointer;
    using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;
    using shape_type = Shape;

  private:
    using basic_ndarray_ref_type = agency::experimental::basic_ndarray_ref<pointer, Shape, Index>;
    using const_basic_ndarray_ref_type = agency::experimental::basic_ndarray_ref<const_pointer, Shape, Index>;

    __AGENCY_ANNOTATION
    basic_ndarray_ref_type& as_basic_ndarray_ref()
    {
      return static_cast<basic_ndarray_ref_type&>(*this);
    }

    __AGENCY_ANNOTATION
    const basic_ndarray_ref_type& as_basic_ndarray_ref() const
    {
      return static_cast<const basic_ndarray_ref_type&>(*this);
    }

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    static basic_ndarray_ref_type allocate_basic_ndarray_ref(allocator_type& alloc, shape_type shape)
    {
      pointer ptr = nullptr;

      if(std::size_t size = agency::detail::index_space_size(shape))
      {
        ptr = alloc.allocate(size);

        if(ptr == nullptr)
        {
          detail::throw_bad_alloc();
        }
      }

      return basic_ndarray_ref_type{ptr, shape};
    }

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    storage(basic_ndarray_ref_type&& basic_ndarray_ref, Allocator&& alloc)
      : basic_ndarray_ref_type{std::move(basic_ndarray_ref)},
        allocator_{std::move(alloc)}
    {
      // leave the other array ref in an empty state
      basic_ndarray_ref = basic_ndarray_ref_type{};
    }

  public:
    __AGENCY_ANNOTATION
    storage(shape_type shape, Allocator&& alloc)
      : storage(allocate_basic_ndarray_ref(alloc, shape), std::move(alloc))
    {}

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    storage(shape_type shape, const allocator_type& alloc = allocator_type{})
      : storage(shape, allocator_type{alloc})
    {}

    __AGENCY_ANNOTATION
    storage(storage&& other)
      : storage(std::move(other.as_basic_ndarray_ref()), std::move(other.allocator()))
    {}

    __AGENCY_ANNOTATION
    storage(const allocator_type& alloc)
      : storage(shape_type{}, alloc)
    {}

    __AGENCY_ANNOTATION
    storage(allocator_type&& alloc)
      : storage(shape_type{}, std::move(alloc))
    {}

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    storage()
      : storage(allocator_type())
    {}

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    ~storage()
    {
      reset();
    }

  private:
    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    void move_assign_allocator(std::true_type, allocator_type& other_alloc)
    {
      // propagate the allocator
      allocator() = std::move(other_alloc);
    }

    __AGENCY_ANNOTATION
    void move_assign_allocator(std::false_type, allocator_type&)
    {
      // do nothing
    }

    __agency_exec_check_disable__
    __AGENCY_ANNOTATION
    void reset()
    {
      if(data() != nullptr)
      {
        allocator().deallocate(data(), size());

        // empty ourself by assigning an empty basic_ndarray_ref_type to the base class
        as_basic_ndarray_ref() = basic_ndarray_ref_type{};
      }
    }

  public:
    __AGENCY_ANNOTATION
    storage& operator=(storage&& other)
    {
      // we have to call reset() instead of simply swapping ourself with other
      // because depending on propagate_on_container_move_assignment, we may need
      // to retain our allocator
      reset();

      move_assign_allocator(typename std::allocator_traits<Allocator>::propagate_on_container_move_assignment(), other.allocator());

      detail::adl_swap(as_basic_ndarray_ref(), other.as_basic_ndarray_ref());

      return *this;
    }

    __AGENCY_ANNOTATION
    basic_ndarray_ref_type all()
    {
      return as_basic_ndarray_ref();
    }

    __AGENCY_ANNOTATION
    const_basic_ndarray_ref_type all() const
    {
      return as_basic_ndarray_ref();
    }

    __AGENCY_ANNOTATION
    pointer data()
    {
      return as_basic_ndarray_ref().data();
    }

    __AGENCY_ANNOTATION
    const_pointer data() const
    {
      return as_basic_ndarray_ref().data();
    }

    __AGENCY_ANNOTATION
    shape_type shape() const
    {
      return as_basic_ndarray_ref().shape();
    }

    __AGENCY_ANNOTATION
    std::size_t size() const
    {
      return as_basic_ndarray_ref().size();
    }

    __AGENCY_ANNOTATION
    const allocator_type& allocator() const
    {
      return allocator_;
    }

    __AGENCY_ANNOTATION
    allocator_type& allocator()
    {
      return allocator_;
    }

    __AGENCY_ANNOTATION
    void swap(storage& other)
    {
      detail::adl_swap(allocator(), other.allocator());
      detail::adl_swap(as_basic_ndarray_ref(), other.as_basic_ndarray_ref());
    }

  private:
    allocator_type allocator_;
};

} // end detail
} // end agency

