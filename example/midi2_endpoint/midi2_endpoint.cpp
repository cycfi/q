/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/midi/packet_reader.hpp>
#include <q/midi/packet_writer.hpp>
#include <q/midi/endpoint.hpp>
#include <q/midi/ci.hpp>
#include <q/midi/profiles.hpp>
#include <q/midi/property_exchange.hpp>
#include <q_io/midi2_stream.hpp>
#include "example.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <random>
#include <span>
#include <string_view>
#include <thread>

///////////////////////////////////////////////////////////////////////////////
// A MIDI 2.0 endpoint, for the MIDI Association's conformance tool and any
// other host to discover. It opens a virtual packet port named "Q Endpoint",
// answers endpoint and function block discovery from a description, and
// answers MIDI-CI discovery, profile configuration and property exchange
// with a MUID of its own, three profiles and three properties. Whatever
// else it is sent, it prints. Run it, then point the MIDI 2.0 Workbench,
// or a DAW, at the port.
//
// Everything above the port is q_lib: the packet reader, the stream
// responder and the three MIDI-CI responders. The port is q_io's pair of
// packet streams, and the output stream is the Sink every responder sends
// through.
///////////////////////////////////////////////////////////////////////////////
namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;
namespace ci = q::midi_ci;

namespace
{
   // Prints what reaches it: notes, and the stream traffic the responder
   // lets through.
   struct monitor : midi2::processor
   {
      using midi2::processor::operator();

      void operator()(midi2::note_on m, std::size_t)
      {
         std::cout << "note on  " << int(m.key())
            << " velocity " << m.velocity() << std::endl;
      }

      void operator()(midi2::note_off m, std::size_t)
      {
         std::cout << "note off " << int(m.key()) << std::endl;
      }

      void operator()(midi::note_on m, std::size_t)
      {
         std::cout << "MIDI 1.0 note on  " << int(m.key())
            << " velocity " << int(m.velocity()) << std::endl;
      }

      void operator()(midi2::stream_configuration_request m, std::size_t)
      {
         std::cout << "stream configuration request, protocol "
            << int(m.protocol()) << std::endl;
      }
   };

   // What this device has to say about itself over property exchange:
   // the mandatory ResourceList, the DeviceInfo it lists, and X-Gain, a
   // manufacturer's resource a host may set.
   struct device
   {
      std::string_view property(std::string_view name) const
      {
         if (name == ci::resource_list)
            return R"([{"resource":"DeviceInfo"})"
                   R"(,{"resource":"X-Gain","canSet":"full")"
                   R"(,"schema":{"title":"Gain","type":"number"}}])";
         if (name == "X-Gain")
            return _gain;
         if (name == "DeviceInfo")
            return R"({"manufacturerId":[125,0,0],"familyId":[1,0])"
                   R"(,"modelId":[1,0],"versionId":[0,1,5,0])"
                   R"(,"manufacturer":"Cycfi","family":"Q")"
                   R"(,"model":"Q Endpoint","version":"1.5"})";
         return {};
      }

      int set_property(std::string_view name, q::byte_span data)
      {
         std::cout << "PE Set " << name << ", " << data.size()
            << " bytes" << std::endl;
         if (name != "X-Gain")
            return ci::pe_reply::not_found;
         if (data.size() > sizeof(_gain_data))
            return ci::pe_reply::bad_data;
         std::copy(data.begin(), data.end(), _gain_data);
         _gain = {reinterpret_cast<char const*>(_gain_data), data.size()};
         std::cout << "   X-Gain = " << _gain << std::endl;
         return ci::pe_reply::ok;
      }

      std::uint8_t      _gain_data[64] = {'1'};
      std::string_view  _gain{"1"};
   };

   // MIDI-CI rides on sysex. This stage hands each sysex to the nested
   // responders, property exchange over profiles over discovery, and
   // packetizes their byte replies onto the output stream. Everything else
   // goes on to the monitor.
   struct ci_stage
   {
      using discovery_type = ci::responder<std::uint32_t(*)()>;
      using profiles_type = ci::profile_responder<discovery_type&>;
      using properties_type = ci::property_responder<device&, profiles_type&>;

      ci_stage(
         midi2::endpoint_description const& d, q::midi2_output_stream& out)
       : _discovery{
            d.identity, &random_muid
          , ci::category::profiles | ci::category::property_exchange
               | ci::category::process_inquiry
          , ci::default_max_sysex_size, d.product_instance_id}
       , _profiles{std::span<ci::profile>{_profile_list}, _discovery}
       , _properties{_device, _profiles}
       , _out{out}
      {}

      // The responders bracket what they send; the packets carry only
      // what lies between.
      auto packetize()
      {
         return [&](q::byte_span bytes)
         {
            midi2::send_sysex7(bytes.subspan(1, bytes.size()-2), _out);
            std::cout << "   -> MIDI-CI, sub id "
               << std::hex << int(bytes[4]) << std::dec << std::endl;
         };
      }

      template <typename Message>
      void operator()(Message msg, std::size_t time) { _next(msg, time); }

      void operator()(midi::sysex_view msg, std::size_t)
      {
         if (msg.universal())
            std::cout << "sysex, universal, sub id "
               << std::hex << int(msg.data()[3]) << std::dec << std::endl;
         _properties(msg, packetize());
      }

      // 5.5: make ourselves known, at start-up and after a new MUID.
      void announce()
      {
         _discovery.announce(packetize());
      }


      std::uint32_t muid() const { return _discovery.muid(); }

      static std::uint32_t random_muid()
      {
         static std::random_device rd;
         return std::uint32_t(rd());
      }

      // Three profiles of a manufacturer's own: one off until a host turns
      // it on, and two permanent, one on and one off, which a host cannot
      // change.
      ci::profile               _profile_list[3] =
      {
         {{0x7D, 0x00, 0x01, 0x01, 0x01}}
       , {{0x7D, 0x00, 0x01, 0x02, 0x01}, ci::to_function_block, true, true}
       , {{0x7D, 0x00, 0x01, 0x03, 0x01}, ci::to_function_block, false, true}
      };
      device                    _device;
      discovery_type            _discovery;
      profiles_type             _profiles;
      properties_type           _properties;
      q::midi2_output_stream&   _out;
      monitor                   _next;
   };

   char const* stream_name(midi2::packet const& p)
   {
      using namespace midi2::stream_status;
      switch ((p.word(0) >> 16) & 0x3FF)
      {
         case endpoint_info:           return "endpoint info";
         case device_identity:         return "device identity";
         case endpoint_name:           return "endpoint name";
         case product_instance_id:     return "product instance id";
         case stream_configuration:    return "stream configuration";
         case function_block_info:     return "function block info";
         case function_block_name:     return "function block name";
         default:                      return "stream message";
      }
   }

   // The output stream, with a line printed for each stream reply.
   struct traced_output
   {
      void send(midi2::packet const& p)
      {
         _out.send(p);
         if (p.message_type() == midi2::message_type::stream)
            std::cout << "   -> " << stream_name(p) << std::endl;
      }

      q::midi2_output_stream& _out;
   };
}

int main()
{
   std::signal(SIGINT, signal_handler);
   std::signal(SIGTERM, signal_handler);

   // What this endpoint says it is. 0x7D is the manufacturer id reserved
   // for prototypes and education; a one byte id goes in the first of the
   // three bytes, so it is written 0x7D0000.
   midi2::function_block const blocks[] =
   {
      {true, midi2::direction::bidirectional, 0, midi2::ui_hint::both
       , 0, 1, ci::version, 0, "Q"}
   };
   midi2::endpoint_description description
   {
      "Q Endpoint", "Q-0001", {0x7D0000, 0x0001, 0x0001, 0x00010500}
    , true, true, false, false, midi2::protocol::midi2, true
    , std::span<midi2::function_block const>{blocks}
   };

   // The port, both directions, on the platform's packet backend.
   q::midi2_output_stream out{"Q Endpoint"};
   q::midi2_input_stream in{"Q Endpoint"};
   if (!out.is_valid() || !in.is_valid())
   {
      std::cerr << "Could not open a virtual packet port." << std::endl;
      return -1;
   }

   // The chain: stream responder, then MIDI-CI over sysex, then the monitor.
   ci_stage stage{description, out};
   traced_output traced{out};
   midi2::stream_responder chain{description, traced, std::ref(stage)};

   std::cout << "Q Endpoint is open on a virtual MIDI 2.0 port. MUID "
      << std::hex << std::setw(7) << std::setfill('0')
      << stage.muid() << std::dec
      << ". Ctrl-C to quit." << std::endl;
   stage.announce();

   while (running)
   {
      in.process(chain);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
   }

   // 5.9.2: a device going away invalidates its MUID, so hosts forget it.
   std::uint8_t bye[32];
   auto const n = ci::make_invalidate_muid(bye, stage.muid(), stage.muid());
   midi2::send_sysex7(q::byte_span{bye+1, n-2}, out);
   std::cout << "Invalidate MUID sent." << std::endl;
   return 0;
}
