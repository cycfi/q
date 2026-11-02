/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_BUFFER_JULY_12_2014)
#define CYCFI_Q_BUFFER_JULY_12_2014

#include <algorithm>
#include <vector>
#include <array>
#include <q/support/base.hpp>
#include <q/detail/init_store.hpp>
#include <q/utility/interpolation.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // ring_buffer
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Storage = std::vector<T>>
   class ring_buffer
   {
   public:

      using value_type = T;
      using storage_type = Storage;
      using index_type = std::size_t;
      using interpolation_type = sample_interpolation::none;

                        explicit ring_buffer();
                        explicit ring_buffer(std::size_t size);
                        ring_buffer(ring_buffer const& rhs) = default;
                        ring_buffer(ring_buffer&& rhs) = default;

      ring_buffer&      operator=(ring_buffer const& rhs) = default;
      ring_buffer&      operator=(ring_buffer&& rhs) = default;

      std::size_t       size() const;
      void              push(T val);
      T const&          front() const;
      T&                front();
      T const&          back() const;
      T&                back();
      T const&          operator[](std::size_t index) const;
      T&                operator[](std::size_t index);
      void              clear();
      void              pop_front();

      Storage&          store();
      const Storage&    store() const;

   private:

      std::size_t	      _mask;
      std::size_t       _pos;
      Storage           _data;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Storage>
   inline ring_buffer<T, Storage>::ring_buffer()
    : _pos(0)
   {
      static_assert(!detail::resizable_container<Storage>::value,
         "Error: Not default constructible for resizable buffers");
      detail::init_store(_data, _mask);
   }

   template <typename T, typename Storage>
   inline ring_buffer<T, Storage>::ring_buffer(std::size_t size)
    : _pos(0)
   {
      static_assert(detail::resizable_container<Storage>::value,
         "Error: Can't be constructed with size. Storage has fixed size.");
      detail::init_store(size, _data, _mask);
   }

   // Get the size of buffer.
   template <typename T, typename Storage>
   inline std::size_t ring_buffer<T, Storage>::size() const
   {
      return _data.size();
   }

   // Push the latest element, overwriting the oldest element.
   template <typename T, typename Storage>
   inline void ring_buffer<T, Storage>::push(T val)
   {
      --_pos &= _mask;
      _data[_pos] = val;
   }

   // Get the latest element.
   template <typename T, typename Storage>
   inline T const& ring_buffer<T, Storage>::front() const
   {
      return (*this)[0];
   }

   // Get the latest element.
   template <typename T, typename Storage>
   inline T& ring_buffer<T, Storage>::front()
   {
      return (*this)[0];
   }

   // Get the oldest element.
   template <typename T, typename Storage>
   inline T const& ring_buffer<T, Storage>::back() const
   {
      return (*this)[size()-1];
   }

   // Get the oldest element.
   template <typename T, typename Storage>
   inline T& ring_buffer<T, Storage>::back()
   {
      return (*this)[size()-1];
   }

   // Get the nth latest element (b[0] is latest element, b[1] is the second
   // and b[size()-1] is the oldest.
   template <typename T, typename Storage>
   inline T const& ring_buffer<T, Storage>::operator[](std::size_t index) const
   {
      return _data[(_pos + index) & _mask];
   }

   // Get the nth latest element (b[0] is latest element, b[1] is the second
   // and b[size()-1] is the oldest.
   template <typename T, typename Storage>
   inline T& ring_buffer<T, Storage>::operator[](std::size_t index)
   {
      return _data[(_pos + index) & _mask];
   }

   // Clear the ring_buffer
   template <typename T, typename Storage>
   inline void ring_buffer<T, Storage>::clear()
   {
      for (auto& e : _data)
         e = T();
   }

   // Remove the front element
   template <typename T, typename Storage>
   inline void ring_buffer<T, Storage>::pop_front()
   {
      ++_pos;
   }

   // Raw access to the data storage
   template <typename T, typename Storage>
   inline Storage& ring_buffer<T, Storage>::store()
   {
      return _data;
   }

   // Raw access to the data storage
   template <typename T, typename Storage>
   inline const Storage& ring_buffer<T, Storage>::store() const
   {
      return _data;
   }

   ////////////////////////////////////////////////////////////////////////////
   // mirrored_ring_buffer: a ring buffer whose first `mirror` elements are
   // repeated after its end, each written twice. A window of up to
   // mirror + 1 elements is then one contiguous stretch of memory
   // regardless of the wrap point: span(age) points at the element `age`
   // back, and the next k elements ascend in age, valid while k <= mirror
   // and age + k < size(). The default mirror is size(): every sample
   // written twice, twice the memory, and any window contiguous, for
   // vector kernels. A short mirror costs only that many extra elements,
   // for a reader that takes a few at a time, such as a 4-point
   // interpolation (mirror 3).
   //
   // No power-of-two rounding: the capacity is exactly what was asked
   // for, and the wraps are explicit compares, one per push and one per
   // read. Reads only: writing through a reference would desync the
   // mirror, so no mutable element access is offered.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Storage = std::vector<T>>
   class mirrored_ring_buffer
   {
   public:

      using value_type = T;
      using storage_type = Storage;
      using index_type = std::size_t;
      using interpolation_type = sample_interpolation::none;

      explicit mirrored_ring_buffer(std::size_t size)
       : mirrored_ring_buffer(size, size)
      {}

      mirrored_ring_buffer(std::size_t size, std::size_t mirror)
       : _size(size)
       , _mirror(std::min(mirror, size))
       , _pos(0)
      {
         static_assert(detail::resizable_container<Storage>::value,
            "Error: Can't be constructed with size. Storage has fixed size.");
         _data.resize(_size + _mirror, T{});
      }

      std::size_t size() const
      {
         return _size;
      }

      void push(T val)
      {
         if (_pos == 0)
            _pos = _size;
         --_pos;
         _data[_pos] = val;
         if (_pos < _mirror)
            _data[_pos + _size] = val;
      }

      T const& front() const          { return (*this)[0]; }
      T const& back() const           { return (*this)[size() - 1]; }

      // The nth latest element.
      T const& operator[](std::size_t index) const
      {
         return *span(index);
      }

      // The elements from `age` back, ascending in age, contiguous.
      T const* span(std::size_t age) const
      {
         auto const i = _pos + age;
         return _data.data() + (i < _size ? i : i - _size);
      }

      std::size_t mirror() const
      {
         return _mirror;
      }

      void clear()
      {
         for (auto& e : _data)
            e = T();
      }

      const Storage& store() const    { return _data; }

   private:

      std::size_t    _size;
      std::size_t    _mirror;
      std::size_t    _pos;
      Storage        _data;
   };
}

#endif
