/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_BUFFER_JULY_12_2014)
#define CYCFI_Q_BUFFER_JULY_12_2014

#include <vector>
#include <array>
#include <q/support/base.hpp>
#include <q/detail/init_store.hpp>

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

   template <typename T, typename Storage>
   inline void ring_buffer<T, Storage>::pop_front()
   {
      ++_pos;
   }
}

#endif
