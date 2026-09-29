/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/audio_stream.hpp>
#include <RtAudio.h>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>

namespace cycfi::q
{
   namespace detail
   {
      RtAudio::Api default_api();
      RtAudio::Api device_api(int id);
   }

   struct audio_stream::impl
   {
      impl(RtAudio::Api api, std::string& error)
       : _audio{api, [&error](auto, auto const& text) { error = text; }}
      {}

      // Device ids are private to each RtAudio instance, so a device is
      // found again, by name, in the instance that opens it. An empty name
      // means the default device.
      unsigned int find(std::string const& name, bool output)
      {
         if (name.empty())
            return output? _audio.getDefaultOutputDevice() : _audio.getDefaultInputDevice();
         for (auto id : _audio.getDeviceIds())
            if (_audio.getDeviceInfo(id).name == name)
               return id;
         return 0;
      }

      // Opens the stream; returns nullptr, with the reason in error, when
      // RtAudio would not.
      static impl* open(
         audio_stream& stream, std::string& error, RtAudio::Api api
       , std::string const& in_name, std::size_t in_channels
       , std::string const& out_name, std::size_t out_channels
       , double sps, int frames
      )
      {
         auto* p = new impl{api, error};
         auto const in_device = in_channels? p->find(in_name, false) : 0;
         auto const out_device = out_channels? p->find(out_name, true) : 0;
         if ((in_channels && !in_device) || (out_channels && !out_device))
         {
            error = "The audio device is not available.";
            delete p;
            return nullptr;
         }

         if (sps == -1)
         {
            auto const info = p->_audio.getDeviceInfo(in_channels? in_device : out_device);
            sps = info.currentSampleRate? info.currentSampleRate : info.preferredSampleRate;
         }
         if (frames == -1)
            frames = 256;      // RtAudio has no "let the host choose"

         RtAudio::StreamParameters in_params;
         in_params.deviceId = in_device;
         in_params.nChannels = in_channels;

         RtAudio::StreamParameters out_params;
         out_params.deviceId = out_device;
         out_params.nChannels = out_channels;

         RtAudio::StreamOptions options;
         options.flags = RTAUDIO_NONINTERLEAVED;

         unsigned int buffer_frames = frames;
         auto const err = p->_audio.openStream(
            out_channels? &out_params : nullptr
          , in_channels? &in_params : nullptr
          , RTAUDIO_FLOAT32, sps, &buffer_frames
          , &impl::callback, &stream, &options
         );
         if (err != RTAUDIO_NO_ERROR)
         {
            if (error.empty())
               error = p->_audio.getErrorText();
            delete p;
            return nullptr;
         }

         // Probe warnings on the way in are not the stream's errors.
         error.clear();
         p->_sps = sps;
         p->_in.resize(in_channels);
         p->_out.resize(out_channels);
         return p;
      }

      // RtAudio hands over non-interleaved audio as one block, channel after
      // channel; process() wants a pointer per channel.
      static int callback(
         void* out_, void* in_, unsigned int frames
       , double, RtAudioStreamStatus, void* user
      )
      {
         auto& self = *static_cast<audio_stream*>(user);
         auto& p = *self._impl;
         auto const start = std::chrono::steady_clock::now();

         auto in = static_cast<float const*>(in_);
         for (std::size_t ch = 0; ch != p._in.size(); ++ch)
            p._in[ch] = in + ch * frames;

         auto out = static_cast<float*>(out_);
         for (std::size_t ch = 0; ch != p._out.size(); ++ch)
            p._out[ch] = out + ch * frames;

         if (!p._in.empty() && !p._out.empty())
            self.process(
               in_channels{p._in.data(), p._in.size(), frames}
             , out_channels{p._out.data(), p._out.size(), frames}
            );
         else if (!p._in.empty())
            self.process(in_channels{p._in.data(), p._in.size(), frames});
         else
            self.process(out_channels{p._out.data(), p._out.size(), frames});

         // The fraction of the buffer's real time spent in the callback,
         // smoothed as PortAudio did.
         auto const spent = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();
         auto const load = spent * p._sps / frames;
         p._cpu_load = 0.9 * p._cpu_load + 0.1 * load;
         return 0;
      }

      RtAudio                    _audio;
      std::vector<float const*>  _in;
      std::vector<float*>        _out;
      double                     _sps = 0;
      std::atomic<double>        _cpu_load{0};
   };

   audio_stream::audio_stream(
      audio_device const& device
    , std::size_t input_channels
    , std::size_t output_channels
    , double sps
    , int frames
   )
    : _input_channels(input_channels)
    , _output_channels(output_channels)
   {
      // The default pseudo device has no name to find.
      auto const name = device.id() == audio_device::default_id?
         std::string{} : device.name();
      _impl = impl::open(
         *this, _error, detail::device_api(device.id())
       , name, input_channels
       , name, output_channels
       , sps, frames
      );
   }

   audio_stream::audio_stream(
      std::size_t input_channels
    , std::size_t output_channels
    , double sps
    , int frames
   )
    : _input_channels(input_channels)
    , _output_channels(output_channels)
   {
      _impl = impl::open(
         *this, _error, detail::default_api()
       , {}, input_channels
       , {}, output_channels
       , sps, frames
      );
   }

   audio_stream::~audio_stream()
   {
      if (is_valid())
      {
         _impl->_audio.closeStream();
         delete _impl;
      }
   }

   void audio_stream::start()
   {
      if (is_valid())
         _impl->_audio.startStream();
   }

   void audio_stream::stop()
   {
      if (is_valid())
         _impl->_audio.stopStream();
   }

   // RtAudio reports one latency figure for the stream: the sum of both
   // directions on a duplex stream, so both queries return it there.
   duration audio_stream::input_latency() const
   {
      if (is_valid() && _input_channels)
         return duration(_impl->_audio.getStreamLatency() / _impl->_sps);
      return {};
   }

   duration audio_stream::output_latency() const
   {
      if (is_valid() && _output_channels)
         return duration(_impl->_audio.getStreamLatency() / _impl->_sps);
      return {};
   }

   double audio_stream::sampling_rate() const
   {
      if (is_valid())
         return _impl->_audio.getStreamSampleRate();
      return 0;
   }

   duration audio_stream::time() const
   {
      if (is_valid())
         return duration{_impl->_audio.getStreamTime()};
      return {};
   }

   double audio_stream::cpu_load() const
   {
      if (is_valid())
         return _impl->_cpu_load;
      return -1;
   }
}
