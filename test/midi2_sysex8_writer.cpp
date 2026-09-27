/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Sending a system exclusive message in its 8 bit form. M2-104-UM version
// 1.0, section 4.5 and Table 20: message type 0x5, a status nibble, a byte
// count that starts at the stream id, the stream id, then thirteen data
// bytes. Every case reads its packets back through packet_reader.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/packet_writer.hpp>
#include <q/midi/packet_reader.hpp>

#include <cstdint>
#include <numeric>
#include <vector>

namespace q = cycfi::q;
namespace midi2 = q::midi_2_0;

namespace
{
   using bytes = std::vector<std::uint8_t>;

   struct recorder : midi2::processor
   {
      using midi2::processor::operator();
      void operator()(midi2::sysex8_view m, std::size_t)
      {
         _sysex.push_back({m.data().begin(), m.data().end()});
         _streams.push_back(m.stream());
      }
      std::vector<bytes>         _sysex;
      std::vector<std::uint8_t>  _streams;
   };

   struct fixture
   {
      void write(
         bytes const& payload, std::uint8_t stream = 0
       , std::uint8_t group = 0)
      {
         midi2::send_sysex8(
            q::byte_span{payload}, stream
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

      std::uint8_t stream_id(std::size_t i) const
      { return (_packets[i].word(0) >> 8) & 0xFF; }

      std::vector<midi2::packet>  _packets;
      midi2::packet_reader<>      _reader;
      recorder                    _rec;
   };

   bytes counting(std::size_t n, std::uint8_t first = 0)
   {
      bytes b(n);
      std::iota(b.begin(), b.end(), first);
      return b;
   }
}

TEST_CASE("4.5 A payload of thirteen bytes or fewer is one complete packet")
{
   fixture f;
   f.write({0x7E, 0x00, 0x06, 0x01});

   REQUIRE(f._packets.size() == 1);
   CHECK(f._packets[0].message_type() == midi2::message_type::data128);
   CHECK(f.form(0) == 0x0);

   // "starting from and including the Stream ID", so four data bytes
   // declare five.
   CHECK(f.count(0) == 5);
   CHECK(f.stream_id(0) == 0);

   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex[0] == bytes{0x7E, 0x00, 0x06, 0x01});
}

TEST_CASE("4.5 Thirteen bytes exactly still fit one packet")
{
   fixture f;
   auto const payload = counting(13);
   f.write(payload);

   REQUIRE(f._packets.size() == 1);
   CHECK(f.form(0) == 0x0);
   CHECK(f.count(0) == 14);              // the maximum the field allows
   CHECK(f._rec._sysex[0] == payload);
}

TEST_CASE("4.5 A longer payload is a start and an end")
{
   fixture f;
   auto const payload = counting(26);
   f.write(payload);

   REQUIRE(f._packets.size() == 2);
   CHECK(f.form(0) == 0x1);
   CHECK(f.form(1) == 0x3);
   CHECK(f.count(0) == 14);
   CHECK(f.count(1) == 14);

   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex[0] == payload);
}

TEST_CASE("4.5 A longer one still takes continues between them")
{
   fixture f;
   auto const payload = counting(27);
   f.write(payload);

   REQUIRE(f._packets.size() == 3);
   CHECK(f.form(0) == 0x1);
   CHECK(f.form(1) == 0x2);
   CHECK(f.form(2) == 0x3);
   CHECK(f.count(2) == 2);               // the stream id and one byte

   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex[0] == payload);
}

TEST_CASE("4.5 The byte count is never zero, even with nothing to send")
{
   // "Stream ID is mandatory (1 byte), so a value of 0x0 is not valid in
   // the # of bytes field."
   fixture f;
   f.write({});

   REQUIRE(f._packets.size() == 1);
   CHECK(f.form(0) == 0x0);
   CHECK(f.count(0) == 1);
   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex[0].empty());
}

TEST_CASE("4.5 Every bit of every byte survives, which is the point")
{
   // The 7 bit form cannot carry these at all.
   fixture f;
   bytes const payload{0xFF, 0x80, 0xAA, 0x7F, 0x00, 0xC3};
   f.write(payload);

   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex[0] == payload);
}

TEST_CASE("4.5 The stream id rides in every packet of the message")
{
   fixture f;
   f.write(counting(27), 5);

   REQUIRE(f._packets.size() == 3);
   for (std::size_t i = 0; i != 3; ++i)
      CHECK(f.stream_id(i) == 5);

   REQUIRE(f._rec._streams.size() == 1);
   CHECK(f._rec._streams[0] == 5);
}

TEST_CASE("4.5 Unused bytes are zero, and the group is where it belongs")
{
   fixture f;
   f.write({0x01, 0x02}, 0, 9);

   REQUIRE(f._packets.size() == 1);
   CHECK(f._packets[0].group() == 9);

   // Two data bytes, so the rest of the packet is padding.
   CHECK(f._packets[0].word(0) == 0x59030001u);
   CHECK(f._packets[0].word(1) == 0x02000000u);
   CHECK(f._packets[0].word(2) == 0u);
   CHECK(f._packets[0].word(3) == 0u);
}

////////////////////////////////////////////////////////////////////////////
// The builder takes a Sink with a send member as readily as a callable.
////////////////////////////////////////////////////////////////////////////
TEST_CASE("A Sink with a send member serves send_sysex8 as well as a callable")
{
   struct member_sink
   {
      void send(midi2::packet const& p) { _sent.push_back(p); }
      std::vector<midi2::packet> _sent;
   };

   bytes const payload{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
   member_sink out;
   midi2::send_sysex8(q::byte_span{payload}, 0, out);

   std::vector<midi2::packet> sent;
   midi2::send_sysex8(
      q::byte_span{payload}, 0
    , [&](midi2::packet const& p) { sent.push_back(p); });

   REQUIRE(!out._sent.empty());
   auto same = [](auto const& a, auto const& b)
   {
      if (a.size() != b.size()) return false;
      for (std::size_t i = 0; i != a.size(); ++i)
         for (std::size_t w = 0; w != 4; ++w)
            if (a[i].word(w) != b[i].word(w)) return false;
      return true;
   };
   CHECK(same(out._sent, sent));
}
