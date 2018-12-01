/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_BUFFER_JULY_12_2014)
#define CYCFI_Q_BUFFER_JULY_12_2014

#include <vector>
#include <array>
#include <q/support.hpp>
#include <q/detail/init_store.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////////////////////////
   // ring_buffer: a simple fixed size ring buffer.
   ////////////////////////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Storage = std::vector<T>>
   class ring_buffer
   {
   public:

      explicit ring_buffer()
       : _pos(0)
      {
         static_assert(!detail::resizable_container<Storage>::value,
            "Error: Not default constructible for resizable buffers");
         detail::init_store(_data, _mask);
      }

      explicit ring_buffer(std::size_t size)
       : _pos(0)
      {
         static_assert(detail::resizable_container<Storage>::value,
            "Error: Can't be constructed with size. Storage has fixed size.");
         detail::init_store(size, _data, _mask);
      }

      ring_buffer(ring_buffer const& rhs) = default;
      ring_buffer(ring_buffer&& rhs) = default;
      ring_buffer& operator=(ring_buffer const& rhs) = default;
      ring_buffer& operator=(ring_buffer&& rhs) = default;

      // size of buffer
      std::size_t size() const
      {
         return _data.size();
      }

      // push the latest element, overwriting the oldest element
      void push(T val)
      {
         --_pos &= _mask;
         _data[_pos] = val;
      }

      // get the nth latest element (b[0] is latest element. b[1] is the second latest)
      T const& operator[](std::size_t index) const
      {
         return _data[(_pos + index) & _mask];
      }

      // get the nth latest element (b[0] is latest element. b[1] is the second latest)
      T& operator[](std::size_t index)
      {
         return _data[(_pos + index) & _mask];
      }

   private:

      std::size_t	_mask;
      std::size_t _pos;
      Storage     _data;
   };
}}

#endif
