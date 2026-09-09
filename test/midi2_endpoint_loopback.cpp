/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// The whole endpoint, end to end, through the operating system: the same
// chain the midi2_endpoint example runs, on virtual packet ports of its
// own, discovered by a probe on the other side of CoreMIDI, ALSA or MIDI
// Services. This is the check the Association's Workbench performs, done
// by a test, and it skips itself where the platform has no packet port.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/packet_reader.hpp>
#include <q/midi/packet_writer.hpp>
#include <q/midi/endpoint.hpp>
#include <q/midi/ci.hpp>
#include <q_io/detail/event_queue.hpp>
#include <libremidi/libremidi.hpp>

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

   struct packet_event
   {
      std::uint32_t  words[4];
      std::size_t    time;
   };

   using queue_type = q::detail::event_queue<packet_event, 256>;

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

   template <typename Port>
   std::optional<Port> find_port(std::vector<Port> const& ports)
   {
      for (auto const& p : ports)
      {
         if (p.port_name.find(port_name) != std::string::npos
            || p.display_name.find(port_name) != std::string::npos)
            return p;
      }
      return {};
   }

   void push(queue_type& queue, libremidi::ump const& u)
   {
      queue.push({{u.data[0], u.data[1], u.data[2], u.data[3]}
                , std::size_t(u.timestamp)});
   }

   template <typename Chain>
   void drain(queue_type& queue, midi2::packet_reader<>& reader, Chain& chain)
   {
      packet_event ev;
      while (queue.pop(ev))
         reader({ev.words[0], ev.words[1], ev.words[2], ev.words[3]}
              , ev.time, chain);
   }

   // The endpoint half: two virtual ports and the chain the conformance
   // suite is about, held together so a test can stand one up in a line.
   struct endpoint
   {
      endpoint(libremidi::API api)
       : _api{api}
       , _out{libremidi::output_configuration{}
            , libremidi::midi_out_configuration_for(api)}
       , _send{[this](midi2::packet const& p)
         {
            std::uint32_t const words[4] =
               {p.word(0), p.word(1), p.word(2), p.word(3)};
            _out.send_ump(words, p.words());
         }}
       , _description{
            port_name, "Q-TEST", {0x7D, 0x0001, 0x0001, 0x00010500}
          , true, true, false, false, midi2::protocol::midi2, true
          , std::span<midi2::function_block const>{_blocks}}
       , _stage{ci::responder{_description.identity, &endpoint_muid}
              , _send, {}}
       , _chain{_description, _send, std::ref(_stage)}
      {}

      bool open()
      {
         if (_out.open_virtual_port(port_name) != stdx::error{})
            return false;
         libremidi::ump_input_configuration config;
         config.on_message = [this](libremidi::ump&& u) { push(_queue, u); };
         config.ignore_sysex = false;     // MIDI-CI rides on sysex
         _in.emplace(config, libremidi::midi_in_configuration_for(_api));
         return _in->open_virtual_port(port_name) == stdx::error{};
      }

      void pump() { drain(_queue, _reader, _chain); }

      using send_type = std::function<void(midi2::packet const&)>;

      libremidi::API                         _api;
      libremidi::midi_out                    _out;
      std::optional<libremidi::midi_in>      _in;
      queue_type                             _queue;
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
      midi2::packet_reader<>                 _reader;
   };
}

TEST_CASE("An endpoint on a virtual packet port answers a probe")
{
   auto const api = libremidi::midi2::default_api();
   if (api == libremidi::API::DUMMY)
   {
      WARN("No MIDI 2.0 backend on this platform; skipping.");
      return;
   }

   endpoint ep{api};
   if (!ep.open())
   {
      WARN("This platform will not open a virtual packet port; skipping.");
      return;
   }

   // The probe finds the endpoint's ports through the system, as a host
   // would. They appear asynchronously.
   libremidi::observer observer{
      libremidi::observer_configuration{
         .track_hardware = true, .track_virtual = true}
    , libremidi::observer_configuration_for(api)};

   std::optional<libremidi::output_port> to_endpoint;
   std::optional<libremidi::input_port> from_endpoint;
   for (int i = 0; i != 40 && !(to_endpoint && from_endpoint); ++i)
   {
      if (!to_endpoint)
         if (auto p = find_port(observer.get_output_ports()))
            to_endpoint.emplace(*p);
      if (!from_endpoint)
         if (auto p = find_port(observer.get_input_ports()))
            from_endpoint.emplace(*p);
      if (!(to_endpoint && from_endpoint))
         std::this_thread::sleep_for(std::chrono::milliseconds(50));
   }
   REQUIRE(to_endpoint.has_value());
   REQUIRE(from_endpoint.has_value());

   queue_type probe_queue;
   libremidi::ump_input_configuration probe_config;
   probe_config.on_message =
      [&](libremidi::ump&& u) { push(probe_queue, u); };
   probe_config.ignore_sysex = false;
   libremidi::midi_in probe_in{
      probe_config, libremidi::midi_in_configuration_for(api)};
   REQUIRE(probe_in.open_port(*from_endpoint) == stdx::error{});

   libremidi::midi_out probe_out{
      libremidi::output_configuration{}
    , libremidi::midi_out_configuration_for(api)};
   REQUIRE(probe_out.open_port(*to_endpoint) == stdx::error{});

   // What a host sends first: endpoint discovery asking for everything,
   // function block discovery for every block, and MIDI-CI discovery.
   probe_out.send_ump(0xF0000101u, 0x1Fu, 0u, 0u);
   probe_out.send_ump(0xF010FF03u, 0u, 0u, 0u);

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
    , [&](midi2::packet const& p)
      {
         std::uint32_t const w[2] = {p.word(0), p.word(1)};
         probe_out.send_ump(w, 2);
      });

   // Pump both ends until every reply is in, or time runs out.
   probe results;
   midi2::packet_reader<> probe_reader;
   auto const deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(3);
   while (std::chrono::steady_clock::now() < deadline)
   {
      ep.pump();
      drain(probe_queue, probe_reader, results);
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

   auto const api = libremidi::midi2::default_api();
   REQUIRE(api != libremidi::API::DUMMY);

   endpoint ep{api};
   REQUIRE(ep.open());
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
