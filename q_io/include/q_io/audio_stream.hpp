/*=============================================================================
   Copyright (c) 2016-2023 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PORT_AUDIO_STREAM_OCTOBER_3_2018)
#define CYCFI_Q_PORT_AUDIO_STREAM_OCTOBER_3_2018

#include <infra/support.hpp>
#include <q_io/audio_device.hpp>
#include <q/support/audio_stream.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   class audio_stream : public audio_stream_base
   {
   public:
                              audio_stream(
                                 std::size_t input_channels
                               , std::size_t output_channels
                               , double sps = -1
                               , int frames = -1
                              );

                              audio_stream(
                                 audio_device const& device
                               , std::size_t input_channels
                               , std::size_t output_channels
                               , double sps = -1
                               , int frames = -1
                              );

      virtual                 ~audio_stream();

      void                    start();
      void                    stop();

      bool                    is_valid() const     { return _impl != nullptr; }
      duration                time() const;
      double                  cpu_load() const;
      char const*             error() const        { return _error; }

      duration                input_latency() const;
      duration                output_latency() const;
      double                  sampling_rate() const;
      std::size_t             input_channels() const  { return _input_channels; }
      std::size_t             output_channels() const { return _output_channels; }

   private:

      struct impl*            _impl;
      std::size_t             _input_channels;
      std::size_t             _output_channels;
      char const*             _error;
   };

   using port_audio_stream [[deprecated("Use audio_stream instead.")]]
      = audio_stream;
}

#endif
