/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// UMP Stream Messages, message type 0xF. From M2-104-UM version 1.1, May
// 11 2023, section 7.1 and Table 33. Cases are named for their clause.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/packet_reader.hpp>
#include <q/midi/ump_stream.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;

namespace
{
   struct recorder : midi2::processor
   {
      using midi2::processor::operator();

      std::vector<std::string>   _seen;
      std::vector<std::uint32_t> _values;
      std::vector<std::string>   _text;

      void operator()(midi2::endpoint_discovery m, std::size_t)
      {
         _seen.push_back("endpoint_discovery");
         _values = {m.version_major(), m.version_minor(), m.filter()
                  , m.wants_info(), m.wants_device_identity(), m.wants_name()
                  , m.wants_product_instance_id()
                  , m.wants_stream_configuration()};
      }

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

      void operator()(midi2::stream_configuration_request m, std::size_t)
      {
         _seen.push_back("stream_configuration_request");
         _values = {m.protocol(), m.receives_jr(), m.transmits_jr()};
      }

      void operator()(midi2::stream_configuration m, std::size_t)
      {
         _seen.push_back("stream_configuration");
         _values = {m.protocol(), m.receives_jr(), m.transmits_jr()};
      }

      void operator()(midi2::function_block_discovery m, std::size_t)
      {
         _seen.push_back("function_block_discovery");
         _values = {m.block(), m.filter(), m.wants_info(), m.wants_name()};
      }

      void operator()(midi2::function_block_info m, std::size_t)
      {
         _seen.push_back("function_block_info");
         _values = {m.active(), m.block(), m.ui_hint(), m.midi1(), m.direction()
                  , m.first_group(), m.groups(), m.midi_ci_version()
                  , m.sysex8_streams()};
      }

      void operator()(midi2::start_of_clip, std::size_t)
      { _seen.push_back("start_of_clip"); }

      void operator()(midi2::end_of_clip, std::size_t)
      { _seen.push_back("end_of_clip"); }

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

      void operator()(midi2::function_block_name_view m, std::size_t)
      {
         _seen.push_back("function_block_name");
         _values = {m.block()};
         _text.push_back(std::string{m.text()});
      }

      void operator()(midi::timing_tick, std::size_t)
      { _seen.push_back("tick"); }
   };

   // Table 33: 0xF, a two bit form, a ten bit status, then the two low
   // bytes of the first word.
   constexpr std::uint32_t stream(
      std::uint8_t form, std::uint16_t status
    , std::uint8_t b2 = 0, std::uint8_t b3 = 0)
   {
      return 0xF0000000u | (std::uint32_t(form) << 26)
         | (std::uint32_t(status) << 16) | (std::uint32_t(b2) << 8) | b3;
   }

   constexpr std::uint32_t word(
      std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d)
   {
      return (std::uint32_t(a) << 24) | (std::uint32_t(b) << 16)
         | (std::uint32_t(c) << 8) | d;
   }

   struct fixture
   {
      void send(midi2::packet const& p) { _reader(p, _time++, _rec); }

      midi2::packet_reader<>  _reader;
      recorder                _rec;
      std::size_t             _time = 0;
   };
}

// The packet itself ///////////////////////////////////////////////////////////

TEST_CASE("2.1.4 Type 0xF is defined in version 1.1, and is four words")
{
   midi2::packet const p{stream(0, 0x00, 1, 1), 0x1Fu, 0u, 0u};
   CHECK(p.defined());
   CHECK(p.words() == 4);
   CHECK(p.message_type() == midi2::message_type::stream);
}

TEST_CASE("7.1 A stream message has a form and a ten bit status, no group")
{
   // "4 bits Message Type with value 0xF, 2 bits Format, 10 bits Status"
   midi2::stream_message const m{
      midi2::packet{stream(2, 0x3FF), 0u, 0u, 0u}};
   CHECK(m.form() == 2);
   CHECK(m.status() == 0x3FF);
}

// 7.1.1 Endpoint discovery /////////////////////////////////////////////////

TEST_CASE("7.1.1 Endpoint Discovery: version and a five bit filter")
{
   // "Each bit set will result in an individual reply."
   fixture f;
   f.send({stream(0, 0x00, 1, 1), 0x1Fu, 0u, 0u});

   REQUIRE(f._rec._seen == std::vector<std::string>{"endpoint_discovery"});
   CHECK(f._rec._values ==
      std::vector<std::uint32_t>{1, 1, 0x1F, 1, 1, 1, 1, 1});
}

TEST_CASE("7.1.1 A filter asking for one thing")
{
   fixture f;
   f.send({stream(0, 0x00, 1, 1), 0x02u, 0u, 0u});      // d only

   CHECK(f._rec._values ==
      std::vector<std::uint32_t>{1, 1, 0x02, 0, 1, 0, 0, 0});
}

// 7.1.2 Endpoint info //////////////////////////////////////////////////////

TEST_CASE("7.1.2 Endpoint Info: static flag, block count, protocols, JR")
{
   // Table 33 word 2: static and count in the top byte, M2 and M1 in the
   // second lowest, JR receive and transmit in the lowest.
   fixture f;
   f.send({stream(0, 0x01, 1, 1), word(0x83, 0x00, 0x11, 0x03), 0u, 0u});

   REQUIRE(f._rec._seen == std::vector<std::string>{"endpoint_info"});
   CHECK(f._rec._values ==
      std::vector<std::uint32_t>{1, 1, 1, 3, 1, 1, 1, 1});
}

TEST_CASE("7.1.2 An endpoint with no function blocks and MIDI 2.0 only")
{
   fixture f;
   f.send({stream(0, 0x01, 1, 1), word(0x00, 0x00, 0x10, 0x00), 0u, 0u});

   CHECK(f._rec._values ==
      std::vector<std::uint32_t>{1, 1, 0, 0, 1, 0, 0, 0});
}

// 7.1.3 Device identity ////////////////////////////////////////////////////

TEST_CASE("7.1.3 Device Identity: manufacturer, family, model, revision")
{
   // "3 bytes Device Manufacturer ... 2 bytes Device Family ... 2 bytes
   // Device Family Model Number ... 4 bytes Software Revision Level".
   // Family and model are LSB first, seven bits each, as Table 33 orders
   // them.
   fixture f;
   f.send({stream(0, 0x02)
         , word(0x00, 0x00, 0x21, 0x09)         // manufacturer 00 21 09
         , word(0x01, 0x02, 0x03, 0x04)         // family 1 | 2<<7, model
         , word(0x01, 0x02, 0x03, 0x04)});      // revision bytes

   REQUIRE(f._rec._seen == std::vector<std::string>{"device_identity"});
   CHECK(f._rec._values[0] == 0x002109u);
   CHECK(f._rec._values[1] == (0x01 | (0x02 << 7)));
   CHECK(f._rec._values[2] == (0x03 | (0x04 << 7)));
   CHECK(f._rec._values[3] == 0x01020304u);
}

// 7.1.4, 7.1.5 Text that may span packets //////////////////////////////////

TEST_CASE("7.1.4 An Endpoint Name in one packet")
{
   // Fourteen bytes of UTF-8 per packet, from the low half of the first
   // word on. "If the name ends in the middle of a UMP, then the remaining
   // data bytes shall be set to 0x00."
   fixture f;
   f.send({stream(0, 0x03, 'Q', 'P'), word('l', 'u', 'g', 0), 0u, 0u});

   REQUIRE(f._rec._seen == std::vector<std::string>{"endpoint_name"});
   CHECK(f._rec._text.front() == "QPlug");
}

TEST_CASE("7.1.4 An Endpoint Name across packets joins in order")
{
   // "one Start UMP (Form = 0x1), up to five optional Continue UMPs (Form =
   // 0x2) and an End UMP (Form = 0x3)"
   fixture f;
   f.send({stream(1, 0x03, 'A', 'B'), word('C', 'D', 'E', 'F')
         , word('G', 'H', 'I', 'J'), word('K', 'L', 'M', 'N')});
   CHECK(f._rec._text.empty());

   f.send({stream(2, 0x03, 'O', 'P'), word('Q', 'R', 'S', 'T')
         , word('U', 'V', 'W', 'X'), word('Y', 'Z', 'a', 'b')});
   f.send({stream(3, 0x03, 'c', 'd'), word('e', 0, 0, 0), 0u, 0u});

   REQUIRE(f._rec._text.size() == 1);
   CHECK(f._rec._text.front() == "ABCDEFGHIJKLMNOPQRSTUVWXYZabcde");
}

TEST_CASE("7.1.4 A full fourteen byte packet has no terminator")
{
   fixture f;
   f.send({stream(0, 0x03, 'A', 'B'), word('C', 'D', 'E', 'F')
         , word('G', 'H', 'I', 'J'), word('K', 'L', 'M', 'N')});

   CHECK(f._rec._text.front() == "ABCDEFGHIJKLMN");
}

TEST_CASE("7.1.5 A Product Instance Id is ASCII in the same shape")
{
   fixture f;
   f.send({stream(0, 0x04, 'S', 'N'), word('-', '0', '0', '1'), 0u, 0u});

   REQUIRE(f._rec._seen == std::vector<std::string>{"product_instance_id"});
   CHECK(f._rec._text.front() == "SN-001");
}

TEST_CASE("A continue or end with no start is ignored")
{
   fixture f;
   f.send({stream(2, 0x03, 'x', 'y'), 0u, 0u, 0u});
   f.send({stream(3, 0x03, 'x', 'y'), 0u, 0u, 0u});

   CHECK(f._rec._text.empty());
}

TEST_CASE("A start of a different text abandons the one in progress")
{
   fixture f;
   f.send({stream(1, 0x03, 'A', 'B'), 0u, 0u, 0u});      // a name begins
   f.send({stream(0, 0x04, 'S', 'N'), 0u, 0u, 0u});      // a complete id
   f.send({stream(3, 0x03, 'C', 0), 0u, 0u, 0u});        // the name ends

   // The id arrived; the name did not, since its start was abandoned.
   CHECK(f._rec._seen == std::vector<std::string>{"product_instance_id"});
}

// 7.1.6 Stream configuration ///////////////////////////////////////////////

TEST_CASE("7.1.6.2 Stream Configuration Request: protocol and JR bits")
{
   // "0x01 MIDI 1.0 Protocol, 0x02 MIDI 2.0 Protocol"; RXJR bit 1, TXJR
   // bit 0.
   fixture f;
   f.send({stream(0, 0x05, 0x02, 0x03), 0u, 0u, 0u});

   REQUIRE(f._rec._seen ==
      std::vector<std::string>{"stream_configuration_request"});
   CHECK(f._rec._values == std::vector<std::uint32_t>{2, 1, 1});
}

TEST_CASE("7.1.6.3 Stream Configuration Notification, MIDI 1.0, no JR")
{
   fixture f;
   f.send({stream(0, 0x06, 0x01, 0x00), 0u, 0u, 0u});

   REQUIRE(f._rec._seen == std::vector<std::string>{"stream_configuration"});
   CHECK(f._rec._values == std::vector<std::uint32_t>{1, 0, 0});
}

// 7.1.7 to 7.1.9 Function blocks ///////////////////////////////////////////

TEST_CASE("7.1.7 Function Block Discovery: block number and filter")
{
   // "Use 0xFF to request information about all Function Blocks."
   fixture f;
   f.send({stream(0, 0x10, 0xFF, 0x03), 0u, 0u, 0u});

   REQUIRE(f._rec._seen ==
      std::vector<std::string>{"function_block_discovery"});
   CHECK(f._rec._values == std::vector<std::uint32_t>{0xFF, 3, 1, 1});
}

TEST_CASE("7.1.8 Function Block Info: every field")
{
   // Word 1: active bit, block number, then UI hint, MIDI 1.0 and direction
   // in two bits each. Word 2: first group, groups spanned, MIDI-CI version,
   // sysex 8 streams.
   fixture f;
   f.send({stream(0, 0x11, 0x82, 0x33), word(1, 2, 1, 0), 0u, 0u});

   REQUIRE(f._rec._seen == std::vector<std::string>{"function_block_info"});
   CHECK(f._rec._values ==
      std::vector<std::uint32_t>{1, 2, 3, 0, 3, 1, 2, 1, 0});
}

TEST_CASE("7.1.9 Function Block Name carries its block number first")
{
   // The block number takes the first data byte, leaving thirteen for the
   // name in each packet.
   fixture f;
   f.send({stream(0, 0x12, 0x01, 'K'), word('e', 'y', 's', 0), 0u, 0u});

   REQUIRE(f._rec._seen == std::vector<std::string>{"function_block_name"});
   CHECK(f._rec._values.front() == 1);
   CHECK(f._rec._text.front() == "Keys");
}

TEST_CASE("7.1.10, 7.1.11 Start and End of Clip")
{
   fixture f;
   f.send({stream(0, 0x20), 0u, 0u, 0u});
   f.send({stream(0, 0x21), 0u, 0u, 0u});

   CHECK(f._rec._seen ==
      std::vector<std::string>{"start_of_clip", "end_of_clip"});
}

// Between the two readers /////////////////////////////////////////////////

TEST_CASE("Plain dispatch delivers the single packet messages")
{
   // The text messages need gathering and belong to packet_reader; the
   // rest are one packet each and dispatch sends them straight through.
   recorder rec;
   midi2::dispatch({stream(0, 0x00, 1, 1), 0x1Fu, 0u, 0u}, 0, rec);
   midi2::dispatch({stream(0, 0x03, 'Q', 'P'), word('l', 'u', 'g', 0), 0u, 0u}
      , 0, rec);

   CHECK(rec._seen == std::vector<std::string>{"endpoint_discovery"});
}

TEST_CASE("4.4.1 A stream message terminates a sysex in progress")
{
   // "If any Message or UMP other than a System Real Time Message is sent
   // after a System Exclusive Start UMP and before the associated System
   // Exclusive End UMP, then that UMP shall terminate the System Exclusive
   // Message." A stream message is such a UMP.
   fixture f;
   f.send({0x30120102u, 0u});                          // sysex start
   f.send({stream(0, 0x20), 0u, 0u, 0u});              // start of clip
   f.send({0x30310300u, 0u});                          // an end, orphaned

   CHECK(f._rec._seen == std::vector<std::string>{"start_of_clip"});
   CHECK(f._reader.drops() == 1);
}
