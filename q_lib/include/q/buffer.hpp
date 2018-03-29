/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_BUFFER_JULY_12_2014)
#define CYCFI_Q_BUFFER_JULY_12_2014

#include <vector>
#include <array>
#include <q/support.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////////////////////////
   // buffer: a simple fixed size buffer using a ring buffer.
   ////////////////////////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      template <typename T>
      void init_store(std::size_t size, std::vector<T>& _data, std::size_t& _mask)
      {
         // allocate the data in a size that is a power of two for efficient indexing
         std::size_t capacity = smallest_pow2(size);
         _mask = capacity - 1;
         _data.resize(capacity, T{});
      }

      template <typename T, std::size_t N>
      void init_store(std::size_t size, std::array<T, N>& _data, std::size_t& _mask)
      {
         // size is ignored; std::array is not resizeable
         _mask = _data.size() - 1;
      }
   }

   template <typename T, typename Storage = std::vector<T>>
   class buffer
   {
   public:

      explicit buffer()
       : _pos(0)
      {
         detail::init_store(-1, _data, _mask);
      }

      explicit buffer(std::size_t size)
       : _pos(0)
      {
         detail::init_store(size, _data, _mask);
      }

      buffer(buffer const& rhs) = default;
      buffer(buffer&& rhs) = default;
      buffer& operator=(buffer const& rhs) = default;
      buffer& operator=(buffer&& rhs) = default;

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
      T operator[](std::size_t index) const
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
