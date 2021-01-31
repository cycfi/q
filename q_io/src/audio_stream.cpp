/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/audio_stream.hpp>
#include <infra/assert.hpp>
#include <portaudio.h>

namespace cycfi::q
{
   namespace detail
   {
      // Case input/output
      int audio_stream_callback1(
         void const* input_
       , void* output_
       , unsigned long frame_count
       , PaStreamCallbackTimeInfo const* time_info
       , PaStreamCallbackFlags status_flags
       , void* user_data
      )
      {
         auto this_ = static_cast<port_audio_stream*>(user_data);
         auto input = reinterpret_cast<float const**>(const_cast<void*>(input_));
         auto output = reinterpret_cast<float**>(output_);

         CYCFI_ASSERT(input && output, "Error! No input and/or output channels.");

         this_->process(
            audio_channels<float const>{ input, this_->input_channels(), frame_count }
            , audio_channels<float>{ output, this_->output_channels(), frame_count }
         );
         return 0;
      }

      // Input only
      int audio_stream_callback2(
         void const* input_
       , void* output_
       , unsigned long frame_count
       , PaStreamCallbackTimeInfo const* time_info
       , PaStreamCallbackFlags status_flags
       , void* user_data
      )
      {
         auto this_ = static_cast<port_audio_stream*>(user_data);
         auto input = reinterpret_cast<float const**>(const_cast<void*>(input_));

         CYCFI_ASSERT(input, "Error! No input channel.");

         this_->process(
            audio_channels<float const>{ input, this_->input_channels(), frame_count }
         );

         return 0;
      }

      // Output only
      int audio_stream_callback3(
         void const* input_
       , void* output_
       , unsigned long frame_count
       , PaStreamCallbackTimeInfo const* time_info
       , PaStreamCallbackFlags status_flags
       , void* user_data
      )
      {
         auto this_ = static_cast<port_audio_stream*>(user_data);
         auto output = reinterpret_cast<float**>(output_);

         CYCFI_ASSERT(output, "Error! No output channel.");

         this_->process(
            audio_channels<float>{ output, this_->output_channels(), frame_count }
         );

         return 0;
      }

      struct port_audio_init;
      port_audio_init const& portaudio_init();
   }

   port_audio_stream::port_audio_stream(
      audio_device const& device
    , std::size_t input_channels
    , std::size_t output_channels
    , int sps
    , int frames
   )
   {
      // Make sure we're initialized
      detail::portaudio_init();

      if (sps == -1)
         sps = device.default_sample_rate();

      if (frames == -1)
         frames = paFramesPerBufferUnspecified;

      _input_channels = input_channels;
      _output_channels = output_channels;

      auto id = device.id();

      PaStreamParameters in_params;
      if (input_channels)
      {
         in_params.channelCount = input_channels;
         in_params.device = id;
         in_params.hostApiSpecificStreamInfo = nullptr;
         in_params.sampleFormat = paFloat32 | paNonInterleaved;
         in_params.suggestedLatency = Pa_GetDeviceInfo(id)->defaultLowInputLatency;
         in_params.hostApiSpecificStreamInfo = nullptr;
      }

      PaStreamParameters out_params;
      if (output_channels)
      {
         out_params.channelCount = output_channels;
         out_params.device = id;
         out_params.hostApiSpecificStreamInfo = nullptr;
         out_params.sampleFormat = paFloat32 | paNonInterleaved;
         out_params.suggestedLatency = Pa_GetDeviceInfo(id)->defaultLowOutputLatency;
         out_params.hostApiSpecificStreamInfo = nullptr;
      }

      auto callback = (input_channels && output_channels)?
         detail::audio_stream_callback1 :
         (input_channels ? detail::audio_stream_callback2 : detail::audio_stream_callback3)
         ;

      auto err = Pa_OpenStream(
         reinterpret_cast<void**>(&_impl)
       , input_channels? &in_params : nullptr
       , output_channels? &out_params : nullptr
       , sps, frames
       , paNoFlag, callback, this
      );
      if (err != paNoError)
      {
         _error = Pa_GetErrorText(err);
         _impl = nullptr;
      }
   }

   port_audio_stream::port_audio_stream(
      std::size_t input_channels
    , std::size_t output_channels
    , int sps
    , int frames
   )
   {
      // Make sure we're initialized
      detail::portaudio_init();

      if (sps == -1)
         sps = Pa_GetDeviceInfo(Pa_GetDefaultInputDevice())->defaultSampleRate;

      if (frames == -1)
         frames = paFramesPerBufferUnspecified;

      _input_channels = input_channels;
      _output_channels = output_channels;

      auto callback = (input_channels && output_channels)?
         detail::audio_stream_callback1 :
         (input_channels ? detail::audio_stream_callback2 : detail::audio_stream_callback3)
         ;

      auto err = Pa_OpenDefaultStream(
         reinterpret_cast<void**>(&_impl)
       , input_channels, output_channels
       , paFloat32 | paNonInterleaved
       , sps, frames
       , callback, this
      );
      if (err != paNoError)
      {
         _error = Pa_GetErrorText(err);
         _impl = nullptr;
      }
   }

   port_audio_stream::~port_audio_stream()
   {
      if (is_valid())
      {
         auto err = Pa_CloseStream(_impl);
         CYCFI_ASSERT(err == paNoError, "Error! Failed to close port_audio_stream.");
      }
   }

   void port_audio_stream::start()
   {
      if (is_valid())
         Pa_StartStream(_impl);
   }

   void port_audio_stream::stop()
   {
      if (is_valid())
         Pa_StopStream(_impl);
   }

   duration port_audio_stream::input_latency() const
   {
      if (is_valid())
         return duration(Pa_GetStreamInfo(_impl)->inputLatency);
      return {};
   }

   duration port_audio_stream::output_latency() const
   {
      if (is_valid())
         return duration(Pa_GetStreamInfo(_impl)->outputLatency);
      return {};
   }

   std::uint32_t port_audio_stream::sampling_rate() const
   {
      if (is_valid())
         return Pa_GetStreamInfo(_impl)->sampleRate;
      return 0;
   }

   duration port_audio_stream::time() const
   {
      if (is_valid())
         return duration{ Pa_GetStreamTime(_impl) };
      return {};
   }

   double port_audio_stream::cpu_load() const
   {
      if (is_valid())
         return Pa_GetStreamCpuLoad(_impl);
      return -1;
   }
}

