/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#include <q_io/midi_device.hpp>
#include <libremidi/libremidi.hpp>
#include <string>
#include <vector>

namespace cycfi::q
{
   struct midi_device::impl
   {
      std::uint32_t  _id;
      std::string    _name;
      std::size_t    _num_inputs;
      std::size_t    _num_outputs;
   };

   std::uint32_t midi_device::id() const
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
      // Enumerating devices is not the same as opening one, so the observer
      // that reports what is plugged in outlives any single listing.
      libremidi::observer& midi_observer()
      {
         // Virtual ports are tracked as well as hardware ones. A DAW, a
         // sequencer, or the IAC driver on macOS all appear as virtual
         // endpoints, and PortMidi listed them, so hiding them here would
         // lose devices Q used to see.
         static libremidi::observer obs{
            libremidi::observer_configuration{
               .track_hardware = true
             , .track_virtual = true
            }
         };
         return obs;
      }

      // The input ports of the last listing, indexed by the id that listing
      // handed out. A stream is opened by id, and only inputs can be opened,
      // so the outputs are not kept.
      std::vector<libremidi::input_port>& input_port_store()
      {
         static std::vector<libremidi::input_port> store;
         return store;
      }

      bool input_port_of(std::uint32_t id, libremidi::input_port& port)
      {
         auto const& store = input_port_store();
         if (id >= store.size())
            return false;
         port = store[id];
         return true;
      }
   }

   std::vector<midi_device> midi_device::list()
   {
      // Held because a midi_device refers to one of these, and because a
      // stream is opened by the id handed out here.
      static std::vector<midi_device::impl> devices;
      devices.clear();

      auto& ports = detail::input_port_store();
      ports.clear();

      // Inputs first, so an input's id also indexes the port store. A port
      // carries MIDI one way, and the two lists a backend reports are
      // unrelated, so each gets an entry of its own.
      std::uint32_t id = 0;
      for (auto const& port : detail::midi_observer().get_input_ports())
      {
         auto const& name =
            port.display_name.empty()? port.port_name : port.display_name;
         devices.push_back({id++, name, 1, 0});
         ports.push_back(port);
      }

      for (auto const& port : detail::midi_observer().get_output_ports())
      {
         auto const& name =
            port.display_name.empty()? port.port_name : port.display_name;
         devices.push_back({id++, name, 0, 1});
      }

      std::vector<midi_device> result;
      result.reserve(devices.size());
      for (auto const& impl : devices)
         result.push_back(impl);
      return result;
   }
}
