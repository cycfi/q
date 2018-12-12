/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/midi_stream.hpp>
#include <portmidi.h>

namespace cycfi { namespace q
{
   namespace detail
   {
      struct port_midi_init;
      port_midi_init const& portmidi_init();
   }

   midi_input_stream::midi_input_stream(midi_device const& device)
   {
      // Make sure we're initialized
      detail::portmidi_init();
      auto err = Pm_OpenInput(
         reinterpret_cast<PortMidiStream**>(&_impl)
       , device.id(), nullptr, 256, nullptr, nullptr);

      if (err != pmNoError)
         _impl = nullptr;
   }

   bool midi_input_stream::next(event& ev)
   {
      if (_impl)
      {
         PmEvent event;
         auto stream = reinterpret_cast<PortMidiStream*>(_impl);
         if (Pm_Read(stream, &event, 1))
         {
            ev.msg = { std::uint32_t(event.message) };
            ev.time = event.timestamp;
            return true;
         }
      }
      return false;
   }
}}

