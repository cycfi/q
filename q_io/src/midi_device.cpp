/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/midi_device.hpp>
#include <libremidi/libremidi.hpp>
#include <deque>
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

      // Every port a protocol has listed, indexed by the id it was given.
      // An entry stays when its port goes, so an id never moves to another
      // port, and the port gets its id back if it returns. A stream is
      // opened by id.
      struct port_entry
      {
         libremidi::port_information   info;
         bool                          input;
         bool                          present;
      };

      using port_store = std::vector<port_entry>;

      port_store& ports_of(midi_device::protocol_type p)
      {
         static port_store byte_ports, packet_ports;
         return p == midi_device::midi_1_0? byte_ports : packet_ports;
      }

      bool same_port(
         port_entry const& entry, port_entry const& found, bool by_handle)
      {
         auto const& a = entry.info;
         auto const& b = found.info;
         return entry.input == found.input
            && a.api == b.api
            && (!by_handle || a.port == b.port)
            && a.port_name == b.port_name
            && a.display_name == b.display_name
            && a.device_name == b.device_name;
      }

      constexpr auto no_id = std::uint32_t(-1);

      // Give each found port that has none yet the id of the first entry
      // not yet claimed by this listing that is the same port.
      void claim_ids(
         port_store& store, std::vector<port_entry> const& found
       , std::vector<std::uint32_t>& ids, bool by_handle)
      {
         for (std::size_t i = 0; i != found.size(); ++i)
         {
            if (ids[i] != no_id)
               continue;
            for (std::uint32_t id = 0; id != store.size(); ++id)
            {
               auto& entry = store[id];
               if (!entry.present && same_port(entry, found[i], by_handle))
               {
                  entry.info = found[i].info;
                  entry.present = true;
                  ids[i] = id;
                  break;
               }
            }
         }
      }

      // The ids of the found ports, in order. A port still there keeps its
      // id. Matching by handle and names first, then by names alone, covers
      // a backend whose handle is an index that shifts when another port
      // goes. A port seen for the first time gets the next new id.
      std::vector<std::uint32_t> assign_ids(
         midi_device::protocol_type p, std::vector<port_entry> const& found)
      {
         auto& store = ports_of(p);
         for (auto& entry : store)
            entry.present = false;

         std::vector<std::uint32_t> ids(found.size(), no_id);
         claim_ids(store, found, ids, true);
         claim_ids(store, found, ids, false);

         for (std::size_t i = 0; i != found.size(); ++i)
         {
            if (ids[i] == no_id)
            {
               ids[i] = std::uint32_t(store.size());
               store.push_back(found[i]);
            }
         }
         return ids;
      }

      bool input_port_of(
         midi_device::protocol_type p, std::uint32_t id
       , libremidi::input_port& port)
      {
         auto const& store = ports_of(p);
         if (id >= store.size() || !store[id].present || !store[id].input)
            return false;
         port = libremidi::input_port{store[id].info};
         return true;
      }

      bool input_port_of(std::uint32_t id, libremidi::input_port& port)
      {
         return input_port_of(midi_device::midi_1_0, id, port);
      }

      bool output_port_of(
         midi_device::protocol_type p, std::uint32_t id
       , libremidi::output_port& port)
      {
         auto const& store = ports_of(p);
         if (id >= store.size() || !store[id].present || store[id].input)
            return false;
         port = libremidi::output_port{store[id].info};
         return true;
      }
   }

   std::vector<midi_device> midi_device::list(protocol_type p)
   {
      // Every device ever listed, indexed by id. A midi_device refers to
      // one of these, so entries are updated in place and never removed:
      // a deque keeps them put as devices come and go.
      static std::deque<midi_device::impl> byte_devices, packet_devices;
      auto& devices = p == midi_1_0? byte_devices : packet_devices;

      // A port carries MIDI one way, and the two lists a backend reports
      // are unrelated, so each port is a device of its own.
      auto& observer = p == midi_1_0?
         detail::midi_observer() : detail::midi2_observer();
      std::vector<detail::port_entry> found;
      for (auto const& port : observer.get_input_ports())
         found.push_back({port, true, true});
      for (auto const& port : observer.get_output_ports())
         found.push_back({port, false, true});

      auto const ids = detail::assign_ids(p, found);

      std::vector<midi_device> result;
      result.reserve(found.size());
      for (std::size_t i = 0; i != found.size(); ++i)
      {
         auto const& info = found[i].info;
         auto const& name =
            info.display_name.empty()? info.port_name : info.display_name;
         auto const input = found[i].input;
         auto const id = ids[i];
         while (devices.size() <= id)
            devices.emplace_back();
         devices[id] = {id, name, input? 1u : 0u, input? 0u : 1u, p};
         result.push_back(devices[id]);
      }
      return result;
   }
}
