/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Sending a system exclusive message as packets. M2-104-UM section 4.4:
// the payload without its 0xF0 and 0xF7, six bytes to a packet, one
// packet marked complete or a start, continues and an end. Every case
// reads its packets back through packet_reader.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/packet_writer.hpp>
#include <q/midi/packet_reader.hpp>

#include <cstdint>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;

namespace
{
   using bytes = std::vector<std::uint8_t>;

   struct recorder : midi2::processor
   {
      using midi2::processor::operator();
      void operator()(midi::sysex_view m, std::size_t)
      {
         _sysex.push_back({m.data().begin(), m.data().end()});
      }
      std::vector<bytes> _sysex;
   };

   struct fixture
   {
      void write(bytes const& payload, std::uint8_t group = 0)
      {
         midi2::send_sysex7(
            std::span<std::uint8_t const>{payload}
          , [&](midi2::packet const& p)
            {
               _packets.push_back(p);
               _reader(p, 0, _rec);
            }
          , group);
      }

      std::uint8_t form(std::size_t i) const
      { return (_packets[i].word(0) >> 20) & 0xF; }

      std::uint8_t count(std::size_t i) const
      { return (_packets[i].word(0) >> 16) & 0xF; }

      std::vector<midi2::packet>  _packets;
      midi2::packet_reader<>      _reader;
      recorder                    _rec;
   };
}

TEST_CASE("4.4 A payload of six bytes or fewer is one complete packet")
{
   fixture f;
   f.write({0x7E, 0x00, 0x06, 0x01});

   REQUIRE(f._packets.size() == 1);
   CHECK(f._packets[0].message_type() == midi2::message_type::data64);
   CHECK(f.form(0) == 0x0);
   CHECK(f.count(0) == 4);
   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front() == bytes{0x7E, 0x00, 0x06, 0x01});
}

TEST_CASE("4.4 Exactly six bytes is still one packet")
{
   fixture f;
   f.write({1, 2, 3, 4, 5, 6});

   REQUIRE(f._packets.size() == 1);
   CHECK(f.form(0) == 0x0);
   CHECK(f.count(0) == 6);
   CHECK(f._rec._sysex.front() == bytes{1, 2, 3, 4, 5, 6});
}

TEST_CASE("4.4 Seven bytes is a start and an end")
{
   fixture f;
   f.write({1, 2, 3, 4, 5, 6, 7});

   REQUIRE(f._packets.size() == 2);
   CHECK(f.form(0) == 0x1);
   CHECK(f.count(0) == 6);
   CHECK(f.form(1) == 0x3);
   CHECK(f.count(1) == 1);
   CHECK(f._rec._sysex.front() == bytes{1, 2, 3, 4, 5, 6, 7});
}

TEST_CASE("4.4 A long payload has continues between")
{
   // The 33 byte Reply to Discovery, minus its brackets, is 31 bytes:
   // start, four continues, end.
   fixture f;
   bytes payload;
   for (std::uint8_t i = 0; i != 31; ++i)
      payload.push_back(i);
   f.write(payload);

   REQUIRE(f._packets.size() == 6);
   CHECK(f.form(0) == 0x1);
   for (std::size_t i = 1; i != 5; ++i)
      CHECK(f.form(i) == 0x2);
   CHECK(f.form(5) == 0x3);
   CHECK(f.count(5) == 1);
   CHECK(f._rec._sysex.front() == payload);
}

TEST_CASE("4.4 An empty payload is one complete packet of no bytes")
{
   fixture f;
   f.write({});

   REQUIRE(f._packets.size() == 1);
   CHECK(f.form(0) == 0x0);
   CHECK(f.count(0) == 0);
   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front().empty());
}

TEST_CASE("Unused bytes are zero, as 4.4 requires")
{
   fixture f;
   f.write({0x7E});

   CHECK((f._packets[0].word(0) & 0xFF) == 0);
   CHECK(f._packets[0].word(1) == 0);
}

TEST_CASE("The group is carried on every packet")
{
   fixture f;
   f.write({1, 2, 3, 4, 5, 6, 7}, 5);

   for (auto const& p : f._packets)
      CHECK(p.group() == 5);
}

TEST_CASE("The brackets are the caller's to strip")
{
   // A MIDI 1.0 sysex builder produces 0xF0 ... 0xF7; the packet form
   // carries only what lies between, 4.4. The writer takes the payload and
   // trusts it.
   std::uint8_t const wire[] = {0xF0, 0x7E, 0x7F, 0x0D, 0x71, 0xF7};
   fixture f;
   f.write(bytes{wire + 1, wire + 5});

   CHECK(f._rec._sysex.front() == bytes{0x7E, 0x7F, 0x0D, 0x71});
}
