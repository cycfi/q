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
      protocol_type  _protocol;
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

   midi_device::protocol_type midi_device::protocol() const
   {
      return _impl._protocol;
   }

   namespace detail
   {
      // Enumerating devices is not the same as opening one, so the observers
      // that report what is plugged in outlive any single listing. There is
      // one per protocol: a system lists its byte ports and its packet ports
      // apart, and libremidi does the same.
      //
      // Virtual ports are tracked as well as hardware ones. A DAW, a
      // sequencer, or the IAC driver on macOS all appear as virtual
      // endpoints, and PortMidi listed them, so hiding them here would
      // lose devices Q used to see.
      libremidi::observer_configuration observer_config()
      {
         return {.track_hardware = true, .track_virtual = true};
      }

      libremidi::observer& midi_observer()
      {
         static libremidi::observer obs{observer_config()};
         return obs;
      }

      libremidi::observer& midi2_observer()
      {
         static libremidi::observer obs{
            observer_config()
          , libremidi::observer_configuration_for(
               libremidi::midi2::default_api())};
         return obs;
      }

      // The ports of the last listing of each protocol, indexed by the id
      // that listing handed out. A stream is opened by id. Only inputs open
      // as byte streams, so the byte outputs are not kept; packet streams
      // open both ways.
      struct port_store
      {
         std::vector<libremidi::input_port>    inputs;
         std::vector<libremidi::output_port>   outputs;
      };

      port_store& ports_of(midi_device::protocol_type p)
      {
         static port_store byte_ports, packet_ports;
         return p == midi_device::midi_1_0? byte_ports : packet_ports;
      }

      bool input_port_of(std::uint32_t id, libremidi::input_port& port)
      {
         auto const& store = ports_of(midi_device::midi_1_0).inputs;
         if (id >= store.size())
            return false;
         port = store[id];
         return true;
      }

      // Packet ids run inputs first, then outputs, as the listing does.
      bool input_port_of(
         midi_device::protocol_type p, std::uint32_t id
       , libremidi::input_port& port)
      {
         auto const& store = ports_of(p).inputs;
         if (id >= store.size())
            return false;
         port = store[id];
         return true;
      }

      bool output_port_of(
         midi_device::protocol_type p, std::uint32_t id
       , libremidi::output_port& port)
      {
         auto const& store = ports_of(p);
         if (id < store.inputs.size())
            return false;
         id -= store.inputs.size();
         if (id >= store.outputs.size())
            return false;
         port = store.outputs[id];
         return true;
      }
   }

   std::vector<midi_device> midi_device::list(protocol_type p)
   {
      // Held because a midi_device refers to one of these, and because a
      // stream is opened by the id handed out here.
      static std::vector<midi_device::impl> byte_devices, packet_devices;
      auto& devices = p == midi_1_0? byte_devices : packet_devices;
      devices.clear();

      auto& observer = p == midi_1_0?
         detail::midi_observer() : detail::midi2_observer();
      auto& ports = detail::ports_of(p);
      ports.inputs.clear();
      ports.outputs.clear();

      // Inputs first, so an input's id also indexes the port store. A port
      // carries MIDI one way, and the two lists a backend reports are
      // unrelated, so each gets an entry of its own.
      std::uint32_t id = 0;
      for (auto const& port : observer.get_input_ports())
      {
         auto const& name =
            port.display_name.empty()? port.port_name : port.display_name;
         devices.push_back({id++, name, 1, 0, p});
         ports.inputs.push_back(port);
      }

      for (auto const& port : observer.get_output_ports())
      {
         auto const& name =
            port.display_name.empty()? port.port_name : port.display_name;
         devices.push_back({id++, name, 0, 1, p});
         ports.outputs.push_back(port);
      }

      std::vector<midi_device> result;
      result.reserve(devices.size());
      for (auto const& impl : devices)
         result.push_back(impl);
      return result;
   }
}
