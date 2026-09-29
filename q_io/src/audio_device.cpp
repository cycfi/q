/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/audio_device.hpp>
#include <RtAudio.h>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace cycfi::q
{
   namespace detail
   {
      // One RtAudio instance drives one API, so enumeration keeps one per
      // compiled API and the device list spans them all.
      std::vector<std::unique_ptr<RtAudio>> const& rtaudio_apis()
      {
         static auto const apis = []
         {
            std::vector<std::unique_ptr<RtAudio>> result;
            std::vector<RtAudio::Api> compiled;
            RtAudio::getCompiledApi(compiled);
            for (auto api : compiled)
               result.push_back(std::make_unique<RtAudio>(api, [](auto, auto const&) {}));
            return result;
         }();
         return apis;
      }

      // The API that knows the device the user chose in the OS: the first
      // with devices, ASIO excepted, since an ASIO driver is an interface
      // and no OS default.
      RtAudio::Api default_api()
      {
         for (auto const& audio : rtaudio_apis())
            if (audio->getCurrentApi() != RtAudio::WINDOWS_ASIO && audio->getDeviceCount())
               return audio->getCurrentApi();
         return rtaudio_apis().empty()? RtAudio::UNSPECIFIED : rtaudio_apis().front()->getCurrentApi();
      }

      // The API of each listed device, by q id, for the stream that opens it.
      std::vector<RtAudio::Api>& device_apis()
      {
         static std::vector<RtAudio::Api> apis;
         return apis;
      }

      RtAudio::Api device_api(int id)
      {
         auto const& apis = device_apis();
         if (id >= 1 && std::size_t(id) <= apis.size())
            return apis[id - 1];
         return default_api();
      }
   }

   struct audio_device::impl
   {
      int            _id;
      RtAudio::Api   _api;
      unsigned int   _rtaudio_id;
      std::string    _name;
      std::size_t    _input_channels;
      std::size_t    _output_channels;
      double         _default_sample_rate;

      // An audio_device holds a reference into this table, so entries are
      // updated in place and never moved: a deque keeps them put as devices
      // come and go. Ids are q's own: RtAudio's are private to an instance.
      static std::deque<impl> const& get_devices()
      {
         static std::deque<impl> devices;

         for (auto const& audio : detail::rtaudio_apis())
         {
            auto const api = audio->getCurrentApi();
            for (auto id : audio->getDeviceIds())
            {
               auto const info = audio->getDeviceInfo(id);
               if (info.inputChannels == 0 && info.outputChannels == 0)
                  continue;

               auto entry = devices.end();
               for (auto i = devices.begin(); i != devices.end(); ++i)
                  if (i->_api == api && i->_rtaudio_id == id)
                     entry = i;
               if (entry == devices.end())
               {
                  entry = devices.insert(devices.end(), impl{});
                  entry->_id = devices.size();
                  entry->_api = api;
                  entry->_rtaudio_id = id;
                  detail::device_apis().push_back(api);
               }

               entry->_name = info.name;
               entry->_input_channels = info.inputChannels;
               entry->_output_channels = info.outputChannels;
               entry->_default_sample_rate = info.currentSampleRate?
                  info.currentSampleRate : info.preferredSampleRate;
            }
         }
         return devices;
      }

      // The pseudo device, default_id: the OS default, resolved when a stream
      // opens it, so it follows the user's choice in the OS from then on.
      static impl const& default_device()
      {
         static impl device = []
         {
            impl d{};
            d._id = default_id;
            d._api = detail::default_api();
            d._rtaudio_id = 0;
            d._name = "Default";
            RtAudio audio{d._api, [](auto, auto const&) {}};
            if (auto id = audio.getDefaultInputDevice())
               d._input_channels = audio.getDeviceInfo(id).inputChannels;
            if (auto id = audio.getDefaultOutputDevice())
            {
               auto const info = audio.getDeviceInfo(id);
               d._output_channels = info.outputChannels;
               d._default_sample_rate = info.currentSampleRate?
                  info.currentSampleRate : info.preferredSampleRate;
            }
            return d;
         }();
         return device;
      }
   };

   int audio_device::id() const
   {
      return _impl._id;
   }

   std::string audio_device::name() const
   {
      return _impl._name;
   }

   std::size_t audio_device::input_channels() const
   {
      return _impl._input_channels;
   }

   std::size_t audio_device::output_channels() const
   {
      return _impl._output_channels;
   }

   double audio_device::default_sample_rate() const
   {
      return _impl._default_sample_rate;
   }

   std::vector<audio_device> audio_device::list()
   {
      std::vector<audio_device> result;
      for (auto const& impl : impl::get_devices())
         result.push_back(impl);
      return result;
   }

   audio_device audio_device::get(int device_id)
   {
      if (device_id == default_id)
         return impl::default_device();
      auto const& devices = impl::get_devices();
      for (auto const& impl : devices)
         if (impl._id == device_id)
            return impl;
      return devices.front();
   }
}
