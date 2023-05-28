/*=============================================================================
   Copyright (c) 2016-2022 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MULTI_BUFFER_OCTOBER_3_2018)
#define CYCFI_Q_MULTI_BUFFER_OCTOBER_3_2018

#include <infra/iterator_range.hpp>
#include <infra/support.hpp>
#include <infra/index_iterator.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   class multi_buffer
   {
   public:

      using sample_type = T;
      using buffer_view = iterator_range<T*>;
      using frames_view = iterator_range<index_iterator>;

                           multi_buffer(
                              T** buffers
                            , std::size_t size
                            , std::size_t frames
                           );

      buffer_view          operator[](std::size_t channel) const;
      std::size_t          size() const;
      frames_view          frames() const;

   private:

      T**                  _buffers;
      std::size_t          _size;
      std::size_t          _frames;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   inline multi_buffer<T>::multi_buffer(T** buffers, std::size_t size, std::size_t frames)
    : _buffers(buffers)
    , _size(size)
    , _frames(frames)
   {}

   template <typename T>
   inline typename multi_buffer<T>::buffer_view
   multi_buffer<T>::operator[](std::size_t channel) const
   {
      T* start = _buffers[channel];
      return { start, start + _frames };
   }

   template <typename T>
   inline std::size_t multi_buffer<T>::size() const
   {
      return _size;
   }

   template <typename T>
   inline typename multi_buffer<T>::frames_view
   multi_buffer<T>::frames() const
   {
      return {{0}, {_frames}};
   }
}

#endif
