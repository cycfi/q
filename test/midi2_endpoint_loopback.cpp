/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// The whole endpoint, end to end, through the operating system: the chain
// a MIDI 2.0 device runs, on virtual packet ports of its own, discovered by
// a probe on the other side of CoreMIDI, ALSA or MIDI Services. This is the
// check the Association's Workbench performs, done by a test, and it skips
// itself where the platform has no packet port.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/packet_reader.hpp>
#include <q/midi/packet_writer.hpp>
#include <q/midi/endpoint.hpp>
#include <q/midi/ci.hpp>
#include <q_io/midi2_stream.hpp>
#include <q_io/midi_device.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;
namespace ci = q::midi_ci;

namespace
{
   char const* port_name = "Q endpoint test";

   // The endpoint side, as the example wires it, minus the printing.
   struct silent : midi2::processor
   {
      using midi2::processor::operator();
   };

   template <typename Send>
   struct ci_stage
   {
      template <typename Message>
      void operator()(Message msg, std::size_t time) { _next(msg, time); }

      void operator()(midi::sysex_view msg, std::size_t)
      {
         _ci(msg, [&](std::span<std::uint8_t const> bytes)
         {
            midi2::send_sysex7(bytes.subspan(1, bytes.size()-2), _send);
         });
      }

      ci::responder<std::uint32_t(*)()>   _ci;
      Send                                _send;
      silent                              _next;
   };

   std::uint32_t endpoint_muid() { return 0x1234567; }

   // The probe side: what came back, decoded by our own readers.
   struct probe : midi2::processor
   {
      using midi2::processor::operator();

      std::vector<std::string>   _seen;
      std::vector<std::string>   _text;
      std::uint8_t               _blocks = 0;
      bool                       _midi2 = false;
      std::uint32_t              _manufacturer = 0;
      std::uint8_t               _protocol = 0;
      std::uint8_t               _ci_sub_id = 0;
      std::uint32_t              _ci_source = 0;
      std::uint32_t              _ci_destination = 0;

      void operator()(midi2::endpoint_info m, std::size_t)
      {
         _seen.push_back("endpoint_info");
         _blocks = m.function_blocks();
         _midi2 = m.midi2();
      }
      void operator()(midi2::device_identity m, std::size_t)
      {
         _seen.push_back("device_identity");
         _manufacturer = m.manufacturer();
      }
      void operator()(midi2::endpoint_name_view m, std::size_t)
      {
         _seen.push_back("endpoint_name");
         _text.push_back(std::string{m.text()});
      }
      void operator()(midi2::product_instance_id_view m, std::size_t)
      {
         _seen.push_back("product_instance_id");
         _text.push_back(std::string{m.text()});
      }
      void operator()(midi2::stream_configuration m, std::size_t)
      {
         _seen.push_back("stream_configuration");
         _protocol = m.protocol();
      }
      void operator()(midi2::function_block_info, std::size_t)
      {
         _seen.push_back("function_block_info");
      }
      void operator()(midi2::function_block_name_view m, std::size_t)
      {
         _seen.push_back("function_block_name");
         _text.push_back(std::string{m.text()});
      }
      void operator()(midi::sysex_view m, std::size_t)
      {
         ci::message_view const v{m.data()};
         if (v.has_header())
         {
            _seen.push_back("midi_ci");
            _ci_sub_id = v.sub_id();
            _ci_source = v.source();
            _ci_destination = v.destination();
         }
      }
   };

   // The device a virtual port shows up as, in the packet listing. It
   // appears asynchronously, so the listing may need a moment.
   std::optional<q::midi_device> wait_for_port(bool input)
   {
      std::optional<q::midi_device> device;
      for (int i = 0; i != 40 && !device; ++i)
      {
         for (auto const& d : q::midi_device::list(q::midi_device::midi_2_0))
         {
            auto const fits = input? d.num_inputs() != 0 : d.num_outputs() != 0;
            if (fits && d.name().find(port_name) != std::string::npos)
            {
               device.emplace(d);
               break;
            }
         }
         if (!device)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
      return device;
   }

   // The endpoint half: two virtual ports and the chain the conformance
   // suite is about, held together so a test can stand one up in a line.
   struct endpoint
   {
      endpoint()
       : _out{port_name}
       , _in{port_name}
       , _send{[this](midi2::packet const& p) { _out.send(p); }}
       , _description{
            port_name, "Q-TEST", {0x7D, 0x0001, 0x0001, 0x00010500}
          , true, true, false, false, midi2::protocol::midi2, true
          , std::span<midi2::function_block const>{_blocks}}
       , _stage{ci::responder{_description.identity, &endpoint_muid}
              , _send, {}}
       , _chain{_description, _send, std::ref(_stage)}
      {}

      bool is_valid() const { return _out.is_valid() && _in.is_valid(); }

      void pump()
      {
         for (int i = 0; i != 64; ++i)
            _in.process(_chain);
      }

      using send_type = std::function<void(midi2::packet const&)>;

      q::midi2_output_stream                 _out;
      q::midi2_input_stream                  _in;
      send_type                              _send;
      midi2::function_block                  _blocks[1] =
      {
         {true, midi2::direction::bidirectional, 0, midi2::ui_hint::both
          , 0, 1, ci::version, 0, "Q"}
      };
      midi2::endpoint_description            _description;
      ci_stage<send_type&>                   _stage;
      midi2::stream_responder<send_type&, std::reference_wrapper<
         ci_stage<send_type&>>>              _chain;
   };
}

TEST_CASE("An endpoint on a virtual packet port answers a probe")
{
   endpoint ep;
   if (!ep.is_valid())
   {
      WARN("This platform will not open a virtual packet port; skipping.");
      return;
   }

   // The probe finds the endpoint's ports through the system, as a host
   // would.
   auto const to_endpoint = wait_for_port(false);
   auto const from_endpoint = wait_for_port(true);
   REQUIRE(to_endpoint.has_value());
   REQUIRE(from_endpoint.has_value());

   q::midi2_input_stream probe_in{*from_endpoint};
   REQUIRE(probe_in.is_valid());
   q::midi2_output_stream probe_out{*to_endpoint};
   REQUIRE(probe_out.is_valid());

   // What a host sends first: endpoint discovery asking for everything,
   // function block discovery for every block, and MIDI-CI discovery.
   probe_out.send(midi2::packet{0xF0000101u, 0x1Fu, 0u, 0u});
   probe_out.send(midi2::packet{0xF010FF03u, 0u, 0u, 0u});

   std::vector<std::uint8_t> discovery{0x7E, 0x7F, 0x0D, 0x70, 0x02};
   auto put_muid = [&](std::uint32_t m)
   {
      for (int i = 0; i != 4; ++i)
         discovery.push_back(std::uint8_t((m >> (7*i)) & 0x7F));
   };
   put_muid(0x0ABCDEF);
   put_muid(ci::broadcast_muid);
   discovery.insert(discovery.end()
                  , {0x00, 0x21, 0x09, 0x02, 0x01, 0x04, 0x03
                   , 0x01, 0x02, 0x03, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00
                   , 0x00});
   midi2::send_sysex7(
      std::span<std::uint8_t const>{discovery}
    , [&](midi2::packet const& p) { probe_out.send(p); });

   // Pump both ends until every reply is in, or time runs out.
   probe results;
   auto const deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(3);
   while (std::chrono::steady_clock::now() < deadline)
   {
      ep.pump();
      for (int i = 0; i != 64; ++i)
         probe_in.process(results);
      if (results._seen.size() >= 8)
         break;
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
   }

   // 7.1.1: five replies to the discovery, in bit order.
   REQUIRE(results._seen.size() >= 8);
   CHECK(results._seen[0] == "endpoint_info");
   CHECK(results._seen[1] == "device_identity");
   CHECK(results._seen[2] == "endpoint_name");
   CHECK(results._seen[3] == "product_instance_id");
   CHECK(results._seen[4] == "stream_configuration");
   CHECK(results._blocks == 1);
   CHECK(results._midi2);
   CHECK(results._manufacturer == 0x7D);
   CHECK(results._protocol == midi2::protocol::midi2);
   REQUIRE(results._text.size() >= 3);
   CHECK(results._text[0] == port_name);
   CHECK(results._text[1] == "Q-TEST");

   // 7.1.7: info and name for the one block.
   CHECK(results._seen[5] == "function_block_info");
   CHECK(results._seen[6] == "function_block_name");
   CHECK(results._text[2] == "Q");

   // MIDI-CI 4.1: a Reply to Discovery, from the endpoint's MUID to ours.
   CHECK(results._seen[7] == "midi_ci");
   CHECK(results._ci_sub_id == ci::sub_id::discovery_reply);
   CHECK(results._ci_source == 0x1234567);
   CHECK(results._ci_destination == 0x0ABCDEF);
}

// The same endpoint, held open for an external host: the Association's
// MIDI 2.0 Workbench, or a DAW. Runs only when asked, for as many seconds
// as Q_MIDI2_ENDPOINT_HOLD says; ctest never sets it.
TEST_CASE("An endpoint holds its port open for an external host")
{
   auto const hold = std::getenv("Q_MIDI2_ENDPOINT_HOLD");
   if (!hold)
      return;

   endpoint ep;
   REQUIRE(ep.is_valid());
   WARN(std::string{"Port \""} + port_name + "\" is open, MUID "
      + std::to_string(endpoint_muid()) + ". Probe it now.");

   auto const deadline = std::chrono::steady_clock::now()
      + std::chrono::seconds(std::atoi(hold));
   while (std::chrono::steady_clock::now() < deadline)
   {
      ep.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
   }
}
