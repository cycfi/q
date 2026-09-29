/*=============================================================================
   Copyright (c) 2016-2023 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MULTI_BUFFER_OCTOBER_3_2018)
#define CYCFI_Q_MULTI_BUFFER_OCTOBER_3_2018

#include <infra/iterator_range.hpp>
#include <infra/support.hpp>
#include <infra/index_iterator.hpp>
#include <q/support/basic_concepts.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   template <std::floating_point T>
   class multi_buffer
   {
   public:

      using sample_type = T;
      using buffer_view = iterator_range<T*>;
      using frames_view = iterator_range<index_iterator>;
      using channels_view = iterator_range<index_iterator>;

                           multi_buffer(
                              T** buffers
                            , std::size_t n_channels
                            , std::size_t n_frames
                           );

      buffer_view          operator[](std::size_t channel) const;
      std::size_t          size() const;

      frames_view          frames;
      channels_view        channels;

   private:

      T**                  _buffers;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   template <std::floating_point T>
   inline multi_buffer<T>::multi_buffer(T** buffers, std::size_t n_channels, std::size_t n_frames)
    : _buffers(buffers)
    , frames({0}, {n_frames})
    , channels({0}, {n_channels})
   {}

   template <std::floating_point T>
   inline typename multi_buffer<T>::buffer_view
   multi_buffer<T>::operator[](std::size_t channel) const
   {
      T* start = _buffers[channel];
      return {start, start + frames.size()};
   }

   template <std::floating_point T>
   inline std::size_t multi_buffer<T>::size() const
   {
      return channels.size();
   }
}

#endif
