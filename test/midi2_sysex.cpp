/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// System exclusive in packets. M2-104-UM section 4.4 and Table 18 for the
// 7 bit form (type 0x3), section 4.5 and Table 20 for the 8 bit form
// (type 0x5). Cases are named for their clause.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/packet_reader.hpp>

#include <cstdint>
#include <string>
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

      void operator()(midi::sysex_view msg, std::size_t)
      {
         _sysex.push_back({msg.data().begin(), msg.data().end()});
         _manufacturers.push_back(msg.manufacturer());
      }

      void operator()(midi2::sysex8_view msg, std::size_t)
      {
         _sysex8.push_back({msg.data().begin(), msg.data().end()});
         _streams.push_back(msg.stream());
      }

      void operator()(midi2::note_on, std::size_t)
      {
         _seen.push_back("note_on");
      }

      void operator()(midi::timing_tick, std::size_t)
      {
         _seen.push_back("tick");
      }

      std::vector<bytes>          _sysex;
      std::vector<std::uint32_t>  _manufacturers;
      std::vector<bytes>          _sysex8;
      std::vector<std::uint8_t>   _streams;
      std::vector<std::string>    _seen;
   };

   struct fixture
   {
      void send(midi2::packet const& p)
      {
         _reader(p, _time++, _rec);
      }

      midi2::packet_reader<>  _reader;
      recorder                _rec;
      std::size_t             _time = 0;
   };

   // Table 18: 0x3 gggg ssss nnnn, then six data bytes, big end first.
   constexpr std::uint32_t sysex7(std::uint8_t status, std::uint8_t count
    , std::uint8_t b3 = 0, std::uint8_t b4 = 0)
   {
      return 0x30000000u | (std::uint32_t(status) << 20)
         | (std::uint32_t(count) << 16) | (std::uint32_t(b3) << 8) | b4;
   }

   constexpr std::uint32_t word(
      std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d)
   {
      return (std::uint32_t(a) << 24) | (std::uint32_t(b) << 16)
         | (std::uint32_t(c) << 8) | d;
   }
}

// 4.4 System exclusive, 7 bit ////////////////////////////////////////////////

TEST_CASE("4.4 A complete message in one packet")
{
   // A universal identity request, 7E 00 06 01, with 0xF0 and 0xF7
   // "discarded": only the payload travels.
   fixture f;
   f.send({sysex7(0x0, 4, 0x7E, 0x00), word(0x06, 0x01, 0, 0)});

   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front() == bytes{0x7E, 0x00, 0x06, 0x01});
   CHECK(f._rec._manufacturers.front() == 0x7E);
}

TEST_CASE("4.4 Start, continue and end join in order")
{
   // "Begin with a System Exclusive Start UMP and terminate with a System
   // Exclusive End UMP. Optional System Exclusive Continue UMPs may be used
   // between"
   fixture f;
   f.send({sysex7(0x1, 6, 0x43, 0x10), word(0x4C, 0x00, 0x00, 0x7E)});
   f.send({sysex7(0x2, 6, 0x01, 0x02), word(0x03, 0x04, 0x05, 0x06)});
   f.send({sysex7(0x3, 2, 0x07, 0x08), word(0, 0, 0, 0)});

   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front() == bytes{
      0x43, 0x10, 0x4C, 0x00, 0x00, 0x7E
    , 0x01, 0x02, 0x03, 0x04, 0x05, 0x06
    , 0x07, 0x08});
}

TEST_CASE("4.4 Only the declared number of bytes is data")
{
   // "This declares the number of valid data bytes in each UMP ... Any
   // unused bytes in the UMP are reserved"
   fixture f;
   f.send({sysex7(0x0, 2, 0x43, 0x10), word(0x55, 0x55, 0x55, 0x55)});

   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front() == bytes{0x43, 0x10});
}

TEST_CASE("4.4 A short start or continue does not end the message")
{
   // "A Start or Continue with fewer than 6 bytes does not signify a
   // message end."
   fixture f;
   f.send({sysex7(0x1, 1, 0x43), 0u});
   f.send({sysex7(0x2, 1, 0x10), 0u});
   CHECK(f._rec._sysex.empty());

   f.send({sysex7(0x3, 0), 0u});
   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front() == bytes{0x43, 0x10});
}

TEST_CASE("4.4.1 A real time message between the packets is allowed")
{
   // "System Real Time Messages and JR Clock Messages may be inserted
   // between the UMPs of a System Exclusive message"
   fixture f;
   f.send({sysex7(0x1, 2, 0x43, 0x10), 0u});
   f.send({0x10F80000u});                   // timing clock
   f.send({0x00000000u});                   // utility NOOP
   f.send({sysex7(0x3, 1, 0x4C), 0u});

   CHECK(f._rec._seen == std::vector<std::string>{"tick"});
   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front() == bytes{0x43, 0x10, 0x4C});
}

TEST_CASE("4.4.1 Any other message terminates the one in progress")
{
   // "If any Message or UMP other than a System Real Time Message is sent
   // after a System Exclusive Start UMP and before the associated System
   // Exclusive End UMP, then that UMP shall terminate the System Exclusive
   // Message." Terminated, not delivered: what was gathered is discarded.
   fixture f;
   f.send({sysex7(0x1, 2, 0x43, 0x10), 0u});
   f.send({0x40903C00u, 0xFFFF0000u});      // a note on
   f.send({sysex7(0x3, 1, 0x4C), 0u});      // an end with no start

   CHECK(f._rec._seen == std::vector<std::string>{"note_on"});
   CHECK(f._rec._sysex.empty());
   CHECK(f._reader.drops() == 1);
}

TEST_CASE("4.4 Continue or end with no start is ignored")
{
   fixture f;
   f.send({sysex7(0x2, 2, 0x43, 0x10), 0u});
   f.send({sysex7(0x3, 2, 0x43, 0x10), 0u});

   CHECK(f._rec._sysex.empty());
}

TEST_CASE("4.4 A message too long for the buffer is dropped whole")
{
   midi2::packet_reader<8> reader;
   recorder rec;

   reader({sysex7(0x1, 6, 1, 1), word(1, 1, 1, 1)}, 0, rec);
   reader({sysex7(0x3, 6, 1, 1), word(1, 1, 1, 1)}, 0, rec);

   CHECK(rec._sysex.empty());
   CHECK(reader.drops() == 1);

   // And it recovers.
   reader({sysex7(0x0, 1, 0x7E), 0u}, 0, rec);
   CHECK(rec._sysex.size() == 1);
}

TEST_CASE("4.4 An empty message is still a message")
{
   fixture f;
   f.send({sysex7(0x0, 0), 0u});

   REQUIRE(f._rec._sysex.size() == 1);
   CHECK(f._rec._sysex.front().empty());
}

TEST_CASE("Other packets go to dispatch as before")
{
   fixture f;
   f.send({0x40903C00u, 0xFFFF0000u});

   CHECK(f._rec._seen == std::vector<std::string>{"note_on"});
}

TEST_CASE("The payload matches what the byte reader gives for the same bytes")
{
   // 4.4: "carry the same data payload as MIDI 1.0 Protocol System
   // Exclusive messages". One message, both ways in, must read alike.
   fixture f;
   f.send({sysex7(0x0, 4, 0x7E, 0x00), word(0x06, 0x01, 0, 0)});

   midi::byte_reader<> reader;
   recorder from_bytes;
   std::uint8_t const wire[] = {0xF0, 0x7E, 0x00, 0x06, 0x01, 0xF7};
   reader(std::span<std::uint8_t const>{wire, 6}, 0, from_bytes);

   REQUIRE(from_bytes._sysex.size() == 1);
   CHECK(from_bytes._sysex.front() == f._rec._sysex.front());
}

// 4.5 System exclusive, 8 bit ////////////////////////////////////////////////

TEST_CASE("4.5 A complete 8 bit message carries a stream id and full bytes")
{
   // Table 20: 0x5 gggg ssss nnnn, stream id, then thirteen data bytes.
   // The count includes the stream id.
   fixture f;
   f.send({0x50040100u | 0xFFu, word(0x80, 0x7F, 0, 0), 0u, 0u});

   REQUIRE(f._rec._sysex8.size() == 1);
   CHECK(f._rec._streams.front() == 1);
   CHECK(f._rec._sysex8.front() == bytes{0xFF, 0x80, 0x7F});
}

TEST_CASE("4.5 An 8 bit message spans packets like the 7 bit one")
{
   fixture f;
   f.send({0x50120100u | 0xAAu, 0u, 0u, 0u});          // start, 1 byte
   f.send({0x50320100u | 0xBBu, 0u, 0u, 0u});          // end, 1 byte

   REQUIRE(f._rec._sysex8.size() == 1);
   CHECK(f._rec._sysex8.front() == bytes{0xAA, 0xBB});
}
