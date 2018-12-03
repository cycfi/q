/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/audio_stream.hpp>
#include <infra/assert.hpp>
#include <portaudio.h>

namespace cycfi { namespace q
{
   namespace detail
   {
      int audio_stream_callback(
         void const* input_
       , void* output_
       , unsigned long frame_count
       , PaStreamCallbackTimeInfo const* time_info
       , PaStreamCallbackFlags status_flags
       , void* user_data
      )
      {
         auto this_ = static_cast<audio_stream*>(user_data);
         auto input = reinterpret_cast<float const**>(const_cast<void*>(input_));
         auto output = reinterpret_cast<float**>(output_);

         if (input && output)
         {
            this_->process(
               audio_channels<float const>{ input, this_->input_channels(), frame_count }
             , audio_channels<float>{ output, this_->output_channels(), frame_count }
            );
         }
         else if (input)
         {
            this_->process(
               audio_channels<float const>{ input, this_->input_channels(), frame_count }
            );
         }
         else
         {
            this_->process(
               audio_channels<float>{ output, this_->output_channels(), frame_count }
            );
         }

         return 0;
      }

      struct port_audio_init;
      port_audio_init const& init();
   }

   audio_stream::audio_stream(
      audio_device const& device
    , std::size_t sps
    , std::size_t input_channels
    , std::size_t output_channels
    , int frames
   )
   {
      // Make sure we're initialized
      detail::init();

      if (frames == -1)
         frames = paFramesPerBufferUnspecified;

      _input_channels = input_channels;
      _output_channels = output_channels;

      auto id = device.id();

      PaStreamParameters in_params;
      in_params.channelCount = input_channels;
      in_params.device = id;
      in_params.hostApiSpecificStreamInfo = nullptr;
      in_params.sampleFormat = paFloat32 | paNonInterleaved;
      in_params.suggestedLatency = Pa_GetDeviceInfo(id)->defaultLowInputLatency;
      in_params.hostApiSpecificStreamInfo = nullptr;

      PaStreamParameters out_params;
      out_params.channelCount = output_channels;
      out_params.device = id;
      out_params.hostApiSpecificStreamInfo = nullptr;
      out_params.sampleFormat = paFloat32 | paNonInterleaved;
      out_params.suggestedLatency = Pa_GetDeviceInfo(id)->defaultLowOutputLatency;
      out_params.hostApiSpecificStreamInfo = nullptr;

      auto err = Pa_OpenStream(
         reinterpret_cast<void**>(&_impl)
       , &in_params, &out_params
       , sps, frames
       , paNoFlag, detail::audio_stream_callback, this
      );
      if (err != paNoError)
         _impl = nullptr;
   }

   audio_stream::audio_stream(
      std::size_t sps
    , std::size_t input_channels
    , std::size_t output_channels
    , int frames
   )
   {
      // Make sure we're initialized
      detail::init();

      if (frames == -1)
         frames = paFramesPerBufferUnspecified;

      _input_channels = input_channels;
      _output_channels = output_channels;

      auto err = Pa_OpenDefaultStream(
         reinterpret_cast<void**>(&_impl)
       , input_channels, output_channels
       , paFloat32 | paNonInterleaved
       , sps, frames
       , detail::audio_stream_callback, this
      );
      if (err != paNoError)
         _impl = nullptr;
   }

   audio_stream::~audio_stream()
   {
      if (is_valid())
      {
         auto err = Pa_CloseStream(_impl);
         CYCFI_ASSERT(err == paNoError, "Error! Failed to close audio_stream.");
      }
   }

   void audio_stream::process(io_buffer const& io, std::size_t ch)
   {
      auto out = io.out.begin();
      for (auto& s : io.in)
         *out++ = s;
   }

   void audio_stream::process(in_channels const& in, out_channels const& out)
   {
      // This only applies for cases where in channels == out channels. If
      // this is not the case, then you should override this member function
      // and process the buffers yourself.

      CYCFI_ASSERT((in.size() == out.size()),
         "Input and output buffers have different number of channels");

      for (std::size_t ch = 0; ch != in.size(); ++ch)
         process(io_buffer{ in[ch], out[ch] }, ch);
   }

   void audio_stream::process(in_channels const& in)
   {
      for (std::size_t ch = 0; ch != in.size(); ++ch)
         process(in[ch], ch);
   }

   void audio_stream::process(out_channels const& out)
   {
      for (std::size_t ch = 0; ch != out.size(); ++ch)
         process(out[ch], ch);
   }

   void audio_stream::start()
   {
      if (is_valid())
         Pa_StartStream(_impl);
   }

   void audio_stream::stop()
   {
      if (is_valid())
         Pa_StopStream(_impl);
   }

   duration audio_stream::input_latency() const
   {
      if (is_valid())
         return duration(Pa_GetStreamInfo(_impl)->inputLatency);
      return {};
   }

   duration audio_stream::output_latency() const
   {
      if (is_valid())
         return duration(Pa_GetStreamInfo(_impl)->outputLatency);
      return {};
   }

   std::uint32_t audio_stream::sampling_rate() const
   {
      if (is_valid())
         return Pa_GetStreamInfo(_impl)->sampleRate;
      return 0;
   }

   duration audio_stream::time() const
   {
      if (is_valid())
         return duration{ Pa_GetStreamTime(_impl) };
      return {};
   }

   double audio_stream::cpu_load() const
   {
      if (is_valid())
         return Pa_GetStreamCpuLoad(_impl);
      return -1;
   }
}}

