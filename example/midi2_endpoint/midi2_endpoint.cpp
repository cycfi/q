/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#include <q/midi/packet_reader.hpp>
#include <q/midi/packet_writer.hpp>
#include <q/midi/endpoint.hpp>
#include <q/midi/ci.hpp>
#include <q_io/detail/event_queue.hpp>
#include <libremidi/libremidi.hpp>
#include "example.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <random>
#include <span>
#include <thread>

///////////////////////////////////////////////////////////////////////////////
// A MIDI 2.0 endpoint, for the MIDI Association's conformance tool and any
// other host to discover. It opens a virtual packet port named "Q Endpoint",
// answers endpoint and function block discovery from a description, answers
// MIDI-CI discovery with a MUID of its own, and prints whatever else it is
// sent. Run it, then point the MIDI 2.0 Workbench, or a DAW, at the port.
//
// Everything above the port is q_lib: the packet reader, the two responders
// and the sysex packetizer. The port itself is libremidi's packet backend,
// which is what q_io will wrap once it has a packet stream of its own.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;
namespace ci = q::midi_ci;

namespace
{
   // Packets cross from libremidi's thread to ours through a queue, as the
   // MIDI 1.0 stream does in q_io.
   struct packet_event
   {
      std::uint32_t  words[4];
      std::size_t    time;
   };

   using queue_type = q::detail::event_queue<packet_event, 1024>;

   // Prints what reaches it: notes, and the MIDI-CI traffic it answers.
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

   // MIDI-CI rides on sysex. This stage hands each sysex to the MIDI-CI
   // responder and turns its byte replies into packets; everything else
   // goes on to the monitor.
   template <typename Send>
   struct ci_stage
   {
      template <typename Message>
      void operator()(Message msg, std::size_t time) { _next(msg, time); }

      void operator()(midi::sysex_view msg, std::size_t)
      {
         if (msg.universal())
            std::cout << "sysex, universal, sub id "
               << std::hex << int(msg.data()[3]) << std::dec << std::endl;

         _ci(msg,
            [&](std::span<std::uint8_t const> bytes)
            {
               // The responder brackets its reply; the packets carry only
               // what lies between.
               midi2::send_sysex7(bytes.subspan(1, bytes.size()-2), _send);
               std::cout << "   -> MIDI-CI reply, sub id "
                  << std::hex << int(bytes[4]) << std::dec << std::endl;
            });
      }

      ci::responder<std::uint32_t(*)()>   _ci;
      Send                                _send;
      monitor                             _next;
   };

   std::uint32_t random_muid()
   {
      static std::random_device device;
      return std::uint32_t(device());
   }

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
}

int main()
{
   signal(SIGINT, signal_handler);
   signal(SIGTERM, signal_handler);

   // What this endpoint says it is. 0x7D is the manufacturer id reserved
   // for prototypes and education.
   midi2::function_block const blocks[] =
   {
      {true, midi2::direction::bidirectional, 0, midi2::ui_hint::both
       , 0, 1, ci::version, 0, "Q"}
   };

   midi2::endpoint_description const description
   {
      "Q Endpoint", "Q-0001", {0x7D, 0x0001, 0x0001, 0x00010500}
    , true, true, false, false, midi2::protocol::midi2, true
    , std::span<midi2::function_block const>{blocks}
   };

   // The port, both directions, on the platform's packet backend.
   auto const api = libremidi::midi2::default_api();

   libremidi::midi_out out{
      libremidi::output_configuration{}
    , libremidi::midi_out_configuration_for(api)};
   if (out.open_virtual_port("Q Endpoint") != stdx::error{})
   {
      std::cerr << "Could not open a virtual packet output." << std::endl;
      return -1;
   }

   auto send = [&](midi2::packet const& p)
   {
      std::uint32_t const words[4] =
         {p.word(0), p.word(1), p.word(2), p.word(3)};
      out.send_ump(words, p.words());
      if (p.message_type() == midi2::message_type::stream)
         std::cout << "   -> " << stream_name(p) << std::endl;
   };

   queue_type queue;
   libremidi::ump_input_configuration input;
   input.on_message = [&](libremidi::ump&& u)
   {
      queue.push({{u.data[0], u.data[1], u.data[2], u.data[3]}
                , std::size_t(u.timestamp)});
   };
   input.timestamps = libremidi::timestamp_mode::Absolute;
   input.ignore_sysex = false;             // MIDI-CI rides on sysex

   libremidi::midi_in in{input, libremidi::midi_in_configuration_for(api)};
   if (in.open_virtual_port("Q Endpoint") != stdx::error{})
   {
      std::cerr << "Could not open a virtual packet input." << std::endl;
      return -1;
   }

   // The chain: stream responder, then MIDI-CI over sysex, then the monitor.
   ci_stage<decltype(send)&> stage{
      ci::responder{description.identity, &random_muid}, send, {}};
   midi2::stream_responder chain{description, send, std::ref(stage)};

   midi2::packet_reader<> reader;

   std::cout << "Q Endpoint is open on a virtual MIDI 2.0 port. MUID "
      << std::hex << std::setw(7) << std::setfill('0')
      << stage._ci.muid() << std::dec
      << ". Ctrl-C to quit." << std::endl;

   while (running)
   {
      packet_event ev;
      while (queue.pop(ev))
      {
         midi2::packet const p{
            ev.words[0], ev.words[1], ev.words[2], ev.words[3]};
         auto const status = (p.word(0) >> 16) & 0x3FF;
         if (p.message_type() == midi2::message_type::stream
            && status == midi2::stream_status::endpoint_discovery)
            std::cout << "endpoint discovery, filter "
               << std::hex << (p.word(1) & 0x1F) << std::dec << std::endl;
         reader(p, ev.time, chain);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
   }
   return 0;
}
