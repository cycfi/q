/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/audio_device.hpp>
#include <infra/assert.hpp>
#include <portaudio.h>
#include <string>

namespace cycfi::q
{
   namespace detail
   {
      struct port_audio_init
      {
         port_audio_init()
         {
            auto err = Pa_Initialize();
            CYCFI_ASSERT(err == paNoError, "Error! Failed to initialize PortAudio.");
         }

         ~port_audio_init()
         {
            auto err = Pa_Terminate();
            CYCFI_ASSERT(err == paNoError, "Error! Failed to terminate PortAudio.");
         }
      };

      port_audio_init const& portaudio_init()
      {
         // This will initialize port audio on first call
         static detail::port_audio_init init_;
         return init_;
      }
   }

   struct audio_device::impl
   {
      uint32_t       _id;
      std::string    _name;
      std::size_t    _input_channels;
      std::size_t    _output_channels;
      std::size_t    _default_sample_rate;

      static std::vector<impl> const& get_devices()
      {
         // Make sure we're initialized
         detail::portaudio_init();
         int num_devices = Pa_GetDeviceCount();

         static std::vector<audio_device::impl> devices;
         devices.reserve(num_devices);

         PaDeviceInfo const* info;
         for (auto i = 0; i < num_devices; ++i)
         {
            info = Pa_GetDeviceInfo(i);
            audio_device::impl impl;
            impl._id = i;
            // copy cheap data over
            impl._input_channels = info->maxInputChannels;
            impl._output_channels = info->maxOutputChannels;
            impl._default_sample_rate = info->defaultSampleRate;
            if (info->maxInputChannels || info->maxOutputChannels)
            {
               if (i >= devices.size()) {
                  impl._name = info->name;
                  devices.push_back(impl);
               } else if (devices[i]._name == info->name) {
                  //device names are unique? change data in place (avoid string copy)
                  devices[i]._id = impl._id;
                  devices[i]._input_channels = impl._input_channels;
                  devices[i]._output_channels = impl._output_channels;
                  devices[i]._default_sample_rate = impl._default_sample_rate;
               } else {
                  //overwrite current device at index
                  devices[i]._id = impl._id;
                  devices[i]._name = info->name;
                  devices[i]._input_channels = impl._input_channels;
                  devices[i]._output_channels = impl._output_channels;
                  devices[i]._default_sample_rate = impl._default_sample_rate;
               }
            }
         }
         return devices;
      }
   };

   uint32_t audio_device::id() const
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

   std::size_t audio_device::default_sample_rate() const
   {
      return _impl._default_sample_rate;
   }

   std::vector<audio_device> audio_device::list()
   {
      auto const& devices = impl::get_devices();
      std::vector<audio_device> result;
      for (auto const& impl : devices)
         result.push_back(impl);
      return std::move(result);
   }

   audio_device audio_device::get(int device_id)
   {
      return impl::get_devices()[device_id];
   }
}

