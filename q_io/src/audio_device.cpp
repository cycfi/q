/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/audio_device.hpp>
#include <infra/assert.hpp>
#include <portaudio.h>
#include <string>

namespace cycfi { namespace q
{
   struct audio_device::impl
   {
      using io_dir = audio_device::io_dir;

      uint32_t             _id;
      std::string          _name;
      std::size_t          _num_channels;
      io_dir               _direction;
   };

   uint32_t audio_device::id() const
   {
      return _impl._id;
   }

   std::string audio_device::name() const
   {
      return _impl._name;
   }

   std::size_t audio_device::num_channels() const
   {
      return _impl._num_channels;
   }

   audio_device::io_dir audio_device::direction() const
   {
      return _impl._direction;
   }

   namespace detail
   {
      struct port_audio_init
      {
         port_audio_init()
         {
            auto err = Pa_Initialize();
            CYCFI_ASSERT(err == paNoError, "Error! Can't initialize port_audio.");
         }

         ~port_audio_init()
         {
            auto err = Pa_Terminate();
            CYCFI_ASSERT(err == paNoError, "Error! Can't terminate port_audio.");
         }
      };
   }

   std::vector<audio_device> audio_device::list()
   {
      // This will initialize port audio on first call
      static detail::port_audio_init init;

      int num_devices = Pa_GetDeviceCount();
      if (num_devices < 0)
         return {};

      static std::vector<audio_device::impl> devices;
      devices.clear();

      PaDeviceInfo const* info;
      for (auto i=0; i < num_devices; ++i)
      {
         info = Pa_GetDeviceInfo(i);
         audio_device::impl impl;
         impl._id = i;
         impl._name = info->name;

         if (info->maxInputChannels)
         {
            impl._num_channels = info->maxInputChannels;
            impl._direction = input;
            devices.push_back(impl);
         }

         if (info->maxOutputChannels)
         {
            impl._num_channels = info->maxOutputChannels;
            impl._direction = output;
            devices.push_back(impl);
         }
      }

      std::vector<audio_device> result;
      for (auto const& impl : devices)
         result.push_back(impl);
      return result;
   }
}}

