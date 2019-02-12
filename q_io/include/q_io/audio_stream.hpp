/*=============================================================================
   Copyright (c) 2016-2019 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_AUDIO_STREAM_OCTOBER_3_2018)
#define CYCFI_Q_AUDIO_STREAM_OCTOBER_3_2018

#include <infra/iterator_range.hpp>
#include <infra/support.hpp>
#include <q_io/audio_device.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   class audio_channels
   {
   public:

      using sample_type = T;

      audio_channels(T** buffers, std::size_t size, std::size_t frames)
       : _buffers(buffers)
       , _size(size)
       , _frames(frames)
      {}

      struct frame_index
      {
         operator std::size_t() const     { return i; }
         operator std::size_t&()          { return i; }
         std::size_t operator*() const    { return i; }
         std::size_t i;
      };

      struct frames_view
      {
         frame_index begin() const        { return { 0 }; }
         frame_index end() const          { return { last }; }
         std::size_t last;
      };

      iterator_range<T*>   operator[](std::size_t channel) const;
      std::size_t          size() const   { return _size; }
      frames_view          frames() const { return { _frames }; }

   private:

      T**                  _buffers;
      std::size_t          _size;
      std::size_t          _frames;
   };

   ////////////////////////////////////////////////////////////////////////////
   class audio_stream
   {
   public:

      using in_channels = audio_channels<float const>;
      using out_channels = audio_channels<float>;

      audio_stream(
         std::size_t input_channels
       , std::size_t output_channels
       , int sps = -1
       , int frames = -1
      );

      audio_stream(
         audio_device const& device
       , std::size_t input_channels
       , std::size_t output_channels
       , int sps = -1
       , int frames = -1
      );

      audio_stream(audio_stream const&) = delete;
      virtual ~audio_stream();
      audio_stream&           operator=(audio_stream const&) = delete;

      void                    start();
      void                    stop();

      virtual void            process(in_channels const& in) {}
      virtual void            process(out_channels const& out) {}
      virtual void            process(in_channels const& in, out_channels const& out) {}

      bool                    is_valid() const     { return _impl != nullptr; }
      duration                time() const;
      double                  cpu_load() const;
      char const*             error() const        { return _error; }

      duration                input_latency() const;
      duration                output_latency() const;
      std::uint32_t           sampling_rate() const;
      std::size_t             input_channels() const  { return _input_channels; }
      std::size_t             output_channels() const { return _output_channels; }

   private:

      struct impl*            _impl;
      std::size_t             _input_channels;
      std::size_t             _output_channels;
      char const*             _error;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   inline iterator_range<T*>
   audio_channels<T>::operator[](std::size_t channel) const
   {
      T* start = _buffers[channel];
      return { start, start + _frames };
   }
}}

#endif
