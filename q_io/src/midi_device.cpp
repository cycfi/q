/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/midi_device.hpp>
#include <infra/assert.hpp>
#include <portmidi.h>
#include <string>

namespace cycfi::q
{
   struct midi_device::impl
   {
      uint32_t       _id;
      std::string    _name;
      std::size_t    _num_inputs;
      std::size_t    _num_outputs;
   };

   uint32_t midi_device::id() const
   {
      return _impl._id;
   }

   std::string midi_device::name() const
   {
      return _impl._name;
   }

   std::size_t midi_device::num_inputs() const
   {
      return _impl._num_inputs;
   }

   std::size_t midi_device::num_outputs() const
   {
      return _impl._num_outputs;
   }

   namespace detail
   {
      struct port_midi_init
      {
         port_midi_init()
         {
            auto err = Pm_Initialize();
            CYCFI_ASSERT(err == pmNoError, "Error! Failed to initialize PortAudio.");
         }

         ~port_midi_init()
         {
            auto err = Pm_Terminate();
            CYCFI_ASSERT(err == pmNoError, "Error! Failed to terminate PortAudio.");
         }
      };

      port_midi_init const& portmidi_init()
      {
         // This will initialize port audio on first call
         static detail::port_midi_init init_;
         return init_;
      }
   }

   std::vector<midi_device> midi_device::list()
   {
      // Make sure we're initialized
      detail::portmidi_init();

      int num_devices = Pm_CountDevices();
      if (num_devices < 0)
         return {};

      static std::vector<midi_device::impl> devices;
      PmDeviceInfo const* info;
      for (auto i=0; i < num_devices; ++i)
      {
         info = Pm_GetDeviceInfo(i);
         midi_device::impl impl;
         impl._id = i;
         impl._name = info->name;
         if (info->input || info->output)
         {
            impl._num_inputs = info->input;
            impl._num_outputs = info->output;
            devices.push_back(impl);
         }
      }

      std::vector<midi_device> result;
      for (auto const& impl : devices)
         result.push_back(impl);
      return std::move(result);
   }
}

