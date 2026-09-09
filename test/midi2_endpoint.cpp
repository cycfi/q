/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// The answering side of the stream messages: what an endpoint sends back
// when discovered. M2-104-UM version 1.1, sections 7.1.1 to 7.1.9. The
// replies are read back through packet_reader, so the builders and the
// readers are checked against each other.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/endpoint.hpp>
#include <q/midi/packet_reader.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;

namespace
{
   // Everything the responder sends, decoded again by our own reader.
   struct decoded : midi2::processor
   {
      using midi2::processor::operator();

      std::vector<std::string>   _seen;
      std::vector<std::uint32_t> _values;
      std::vector<std::string>   _text;
      std::vector<std::uint8_t>  _blocks;

      void operator()(midi2::endpoint_info m, std::size_t)
      {
         _seen.push_back("endpoint_info");
         _values = {m.version_major(), m.version_minor()
                  , m.static_function_blocks(), m.function_blocks()
                  , m.midi2(), m.midi1(), m.receives_jr(), m.transmits_jr()};
      }

      void operator()(midi2::device_identity m, std::size_t)
      {
         _seen.push_back("device_identity");
         _values = {m.manufacturer(), m.family(), m.model(), m.revision()};
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
         _values = {m.protocol(), m.receives_jr(), m.transmits_jr()};
      }

      void operator()(midi2::function_block_info m, std::size_t)
      {
         _seen.push_back("function_block_info");
         _blocks.push_back(m.block());
         _values = {m.active(), m.block(), m.ui_hint(), m.midi1()
                  , m.direction(), m.first_group(), m.groups()
                  , m.midi_ci_version(), m.sysex8_streams()};
      }

      void operator()(midi2::function_block_name_view m, std::size_t)
      {
         _seen.push_back("function_block_name");
         _blocks.push_back(m.block());
         _text.push_back(std::string{m.text()});
      }
   };

   struct sink
   {
      void operator()(midi2::packet const& p)
      {
         _packets.push_back(p);
         _reader(p, 0, _decoded);
      }

      std::vector<midi2::packet>  _packets;
      midi2::packet_reader<>      _reader;
      decoded                     _decoded;
   };

   // What is not a stream message goes through to here.
   struct synth : midi2::processor
   {
      using midi2::processor::operator();
      void operator()(midi2::note_on, std::size_t) { ++_notes; }
      int _notes = 0;
   };

   constexpr std::uint32_t stream(
      std::uint8_t form, std::uint16_t status
    , std::uint8_t b2 = 0, std::uint8_t b3 = 0)
   {
      return 0xF0000000u | (std::uint32_t(form) << 26)
         | (std::uint32_t(status) << 16) | (std::uint32_t(b2) << 8) | b3;
   }

   midi2::function_block const blocks[] =
   {
      {true, midi2::direction::bidirectional, 0, midi2::ui_hint::both
       , 0, 1, 0x02, 0, "Keys"}
    , {true, midi2::direction::output, 0, midi2::ui_hint::sender
       , 1, 2, 0x02, 0, "A long function block name here"}
   };

   struct fixture
   {
      fixture(std::string_view name = "Q Endpoint"
            , std::string_view id = "SN-001")
       : _description{
            name, id, {0x002109, 0x0102, 0x0304, 0x01020304}
          , true, true, false, false, midi2::protocol::midi2, true
          , std::span<midi2::function_block const>{blocks}}
       , _chain{_description, std::ref(_sink), std::ref(_synth)}
      {}

      void send(midi2::packet const& p)
      {
         _reader(p, _time++, _chain);
      }

      midi2::endpoint_description             _description;
      sink                                    _sink;
      synth                                   _synth;
      midi2::stream_responder<sink&, synth&>  _chain;
      midi2::packet_reader<>                  _reader;
      std::size_t                             _time = 0;
   };
}

TEST_CASE("7.1.2 Endpoint discovery with 'e' set gets an Endpoint Info")
{
   // "A UMP Endpoint shall send an Endpoint Info Notification after
   // receiving and in reply to an Endpoint Discovery message with the 'e'
   // bit set". Version 1.1, two static function blocks, both protocols,
   // no timestamps.
   fixture f;
   f.send({stream(0, 0x00, 1, 1), 0x01u, 0u, 0u});

   REQUIRE(f._sink._decoded._seen ==
      std::vector<std::string>{"endpoint_info"});
   CHECK(f._sink._decoded._values ==
      std::vector<std::uint32_t>{1, 1, 1, 2, 1, 1, 0, 0});
}

TEST_CASE("7.1.3 'd' gets a Device Identity")
{
   fixture f;
   f.send({stream(0, 0x00, 1, 1), 0x02u, 0u, 0u});

   REQUIRE(f._sink._decoded._seen ==
      std::vector<std::string>{"device_identity"});
   CHECK(f._sink._decoded._values ==
      std::vector<std::uint32_t>{0x002109, 0x0102, 0x0304, 0x01020304});
}

TEST_CASE("7.1.4 'n' gets the Endpoint Name, whole, in one packet")
{
   // Ten bytes fits the fourteen a complete packet holds.
   fixture f;
   f.send({stream(0, 0x00, 1, 1), 0x04u, 0u, 0u});

   REQUIRE(f._sink._packets.size() == 1);
   REQUIRE(f._sink._decoded._text.size() == 1);
   CHECK(f._sink._decoded._text.front() == "Q Endpoint");
}

TEST_CASE("7.1.4 A longer name spans packets and comes back whole")
{
   // "comprised of one Start UMP, up to five optional Continue UMPs and an
   // End UMP". Thirty bytes is three packets.
   fixture f{"An endpoint name of thirty byt"};
   f.send({stream(0, 0x00, 1, 1), 0x04u, 0u, 0u});

   REQUIRE(f._sink._packets.size() == 3);
   CHECK(((f._sink._packets[0].word(0) >> 26) & 3) == 1);     // start
   CHECK(((f._sink._packets[1].word(0) >> 26) & 3) == 2);     // continue
   CHECK(((f._sink._packets[2].word(0) >> 26) & 3) == 3);     // end
   REQUIRE(f._sink._decoded._text.size() == 1);
   CHECK(f._sink._decoded._text.front() == "An endpoint name of thirty byt");
}

TEST_CASE("7.1.4 A name of exactly fourteen bytes is one complete packet")
{
   fixture f{"Fourteen bytes"};
   f.send({stream(0, 0x00, 1, 1), 0x04u, 0u, 0u});

   REQUIRE(f._sink._packets.size() == 1);
   CHECK(f._sink._decoded._text.front() == "Fourteen bytes");
}

TEST_CASE("7.1.4 A name is cut at 98 bytes, the most the message allows")
{
   std::string const long_name(120, 'x');
   fixture f{long_name};
   f.send({stream(0, 0x00, 1, 1), 0x04u, 0u, 0u});

   REQUIRE(f._sink._decoded._text.size() == 1);
   CHECK(f._sink._decoded._text.front().size() == 98);
   CHECK(f._sink._packets.size() == 7);      // start, five continues, end
}

TEST_CASE("7.1.5 'i' gets the Product Instance Id")
{
   fixture f;
   f.send({stream(0, 0x00, 1, 1), 0x08u, 0u, 0u});

   REQUIRE(f._sink._decoded._seen ==
      std::vector<std::string>{"product_instance_id"});
   CHECK(f._sink._decoded._text.front() == "SN-001");
}

TEST_CASE("7.1.6.3 's' gets a Stream Configuration Notification")
{
   fixture f;
   f.send({stream(0, 0x00, 1, 1), 0x10u, 0u, 0u});

   REQUIRE(f._sink._decoded._seen ==
      std::vector<std::string>{"stream_configuration"});
   CHECK(f._sink._decoded._values == std::vector<std::uint32_t>{2, 0, 0});
}

TEST_CASE("7.1.1 Every bit set gets every reply, in order")
{
   // "Each bit set will result in an individual reply."
   fixture f;
   f.send({stream(0, 0x00, 1, 1), 0x1Fu, 0u, 0u});

   CHECK(f._sink._decoded._seen == std::vector<std::string>{
      "endpoint_info", "device_identity", "endpoint_name"
    , "product_instance_id", "stream_configuration"});
}

TEST_CASE("7.1.1 A discovery from a newer version is still answered")
{
   // "If the received version is higher than the Device's supported
   // version, the Device shall only process the fields defined for its
   // supported version"
   fixture f;
   f.send({stream(0, 0x00, 2, 0), 0x01u, 0u, 0u});

   CHECK(f._sink._decoded._seen == std::vector<std::string>{"endpoint_info"});
}

TEST_CASE("7.1.6.2 A Stream Configuration Request is confirmed")
{
   // "If a Stream Configuration Request message was sent the Receiver
   // replies with a Stream Configuration Notification message to confirm
   // the requested configuration."
   fixture f;
   f.send({stream(0, 0x05, 0x01, 0x00), 0u, 0u, 0u});     // MIDI 1.0

   REQUIRE(f._sink._decoded._seen ==
      std::vector<std::string>{"stream_configuration"});
   CHECK(f._sink._decoded._values == std::vector<std::uint32_t>{1, 0, 0});
   CHECK(f._chain.protocol() == midi2::protocol::midi1);
}

TEST_CASE("7.1.6.2 A request the endpoint cannot meet gets its current state")
{
   // "If the Receiver is unable to change some or all parts of the
   // configuration, the Receiver replies with its current configuration."
   // Timestamps are not offered, and a reserved protocol is not a protocol.
   fixture f;
   f.send({stream(0, 0x05, 0x02, 0x03), 0u, 0u, 0u});     // with JR
   CHECK(f._sink._decoded._values == std::vector<std::uint32_t>{2, 0, 0});

   f.send({stream(0, 0x05, 0x03, 0x00), 0u, 0u, 0u});     // reserved
   CHECK(f._sink._decoded._values == std::vector<std::uint32_t>{2, 0, 0});
   CHECK(f._chain.protocol() == midi2::protocol::midi2);
}

TEST_CASE("7.1.8 Function Block Discovery of one block with 'i'")
{
   fixture f;
   f.send({stream(0, 0x10, 0x01, 0x01), 0u, 0u, 0u});

   REQUIRE(f._sink._decoded._seen ==
      std::vector<std::string>{"function_block_info"});
   CHECK(f._sink._decoded._values == std::vector<std::uint32_t>{
      1, 1, midi2::ui_hint::sender, 0, midi2::direction::output
    , 1, 2, 0x02, 0});
}

TEST_CASE("7.1.9 Function Block Discovery with 'n' gets the name")
{
   fixture f;
   f.send({stream(0, 0x10, 0x00, 0x02), 0u, 0u, 0u});

   REQUIRE(f._sink._decoded._seen ==
      std::vector<std::string>{"function_block_name"});
   CHECK(f._sink._decoded._blocks.front() == 0);
   CHECK(f._sink._decoded._text.front() == "Keys");
}

TEST_CASE("7.1.9 A block name longer than thirteen bytes spans packets")
{
   fixture f;
   f.send({stream(0, 0x10, 0x01, 0x02), 0u, 0u, 0u});

   REQUIRE(f._sink._packets.size() == 3);
   CHECK(f._sink._decoded._blocks.front() == 1);
   CHECK(f._sink._decoded._text.front() == "A long function block name here");
}

TEST_CASE("7.1.7 0xFF asks about all blocks, info then name for each")
{
   // "If the Function Block Discovery message has the Function Block
   // Number field set to 0xFF, then the reply shall be one ... per
   // Function Block."
   fixture f;
   f.send({stream(0, 0x10, 0xFF, 0x03), 0u, 0u, 0u});

   CHECK(f._sink._decoded._seen == std::vector<std::string>{
      "function_block_info", "function_block_name"
    , "function_block_info", "function_block_name"});
   CHECK(f._sink._decoded._blocks == std::vector<std::uint8_t>{0, 0, 1, 1});
}

TEST_CASE("7.1.7 A block that does not exist gets nothing")
{
   fixture f;
   f.send({stream(0, 0x10, 0x05, 0x03), 0u, 0u, 0u});

   CHECK(f._sink._packets.empty());
}

TEST_CASE("Everything that is not a stream message passes through")
{
   fixture f;
   f.send({0x40903C00u, 0xFFFF0000u});
   f.send({stream(0, 0x20), 0u, 0u, 0u});     // start of clip, not ours

   CHECK(f._synth._notes == 1);
   CHECK(f._sink._packets.empty());
}
