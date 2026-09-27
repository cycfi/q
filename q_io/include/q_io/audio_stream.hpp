/*=============================================================================
   Copyright (c) 2016-2023 Cycfi Research. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_PORT_AUDIO_STREAM_OCTOBER_3_2018)
#define CYCFI_Q_PORT_AUDIO_STREAM_OCTOBER_3_2018

#include <infra/support.hpp>
#include <q_io/audio_device.hpp>
#include <q/support/audio_stream.hpp>
#include <string>

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
      char const*             error() const        { return _error.c_str(); }

      duration                input_latency() const;
      duration                output_latency() const;
      double                  sampling_rate() const;
      std::size_t             input_channels() const  { return _input_channels; }
      std::size_t             output_channels() const { return _output_channels; }

   private:

      struct impl;
      impl*                   _impl;
      std::size_t             _input_channels;
      std::size_t             _output_channels;
      std::string             _error;
   };

   using port_audio_stream [[deprecated("Use audio_stream instead.")]]
      = audio_stream;
}

#endif
