/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// From Universal MIDI Packet (UMP) Format and MIDI 2.0 Protocol, version
// 1.0, February 20 2020 (MMA/AMEI M2-104-UM). Sections named per case.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/ump.hpp>

#include <cstdint>

namespace q = cycfi::q;
namespace midi2 = q::midi_2_0;

TEST_CASE("2.1.2 The top four bits are the message type")
{
   // "The most significant 4 bits in every UMP shall contain the Message
   // Type field"
   midi2::packet const p{0x40903C00u, 0xFFFF0000u};

   CHECK(p.message_type() == 0x4);
}

TEST_CASE("2.1.2 The next four bits are the group")
{
   // "A 4-bit Group field is next, addressing every UMP Format MIDI
   // Message ... to one of 16 Groups."
   CHECK(midi2::packet{0x40903C00u}.group() == 0x0);
   CHECK(midi2::packet{0x4F903C00u}.group() == 0xF);
   CHECK(midi2::packet{0x27903C00u}.group() == 0x7);
}

TEST_CASE("2.1.4 Each message type has one size")
{
   // Table 3, in words rather than bits.
   struct { std::uint8_t type; std::size_t words; } const cases[] =
   {
      {0x0, 1}, {0x1, 1}, {0x2, 1}, {0x3, 2}, {0x4, 2}, {0x5, 4}
    , {0x6, 1}, {0x7, 1}, {0x8, 2}, {0x9, 2}, {0xA, 2}, {0xB, 3}
    , {0xC, 3}, {0xD, 4}, {0xE, 4}, {0xF, 4}
   };

   for (auto const& c : cases)
      CHECK(midi2::packet_words(c.type) == c.words);
}

TEST_CASE("2.1.4 A packet knows its own size")
{
   CHECK(midi2::packet{0x20903C40u}.words() == 1);        // MIDI 1.0 voice
   CHECK(midi2::packet{0x40903C00u, 0u}.words() == 2);    // MIDI 2.0 voice
   CHECK(midi2::packet{0x50000000u, 0u, 0u, 0u}.words() == 4);
}

TEST_CASE("2.1.2 A status field is read where its type puts it")
{
   // Type 0x4 carries an 8 bit status: the four bits of status and the
   // four of channel, per Figure 17.
   midi2::packet const voice{0x41923C00u, 0u};
   CHECK(voice.status() == 0x9);
   CHECK(voice.channel() == 0x2);

   // Type 0x1 carries its status in the whole byte, with no channel.
   midi2::packet const system{0x11F80000u};
   CHECK(system.system_status() == 0xF8);
}

TEST_CASE("A 32 bit packet is built from one word")
{
   auto const p = midi2::packet{0x21B00740u};

   CHECK(p.words() == 1);
   CHECK(p.word(0) == 0x21B00740u);
}

TEST_CASE("A 64 bit packet keeps both words in order")
{
   auto const p = midi2::packet{0x40903C00u, 0xFFFF0000u};

   CHECK(p.word(0) == 0x40903C00u);
   CHECK(p.word(1) == 0xFFFF0000u);
}

TEST_CASE("2.1.4 Types 0x6, 0x7 and the rest are reserved but sized")
{
   // "Messages marked as Reserved shall not be used", but a receiver still
   // has to know how far to skip, which is what the size table is for.
   CHECK(midi2::packet{0x60000000u}.words() == 1);
   CHECK(midi2::packet{0xB0000000u, 0u, 0u}.words() == 3);
   CHECK_FALSE(midi2::packet{0x60000000u}.defined());
   CHECK(midi2::packet{0x40903C00u, 0u}.defined());
}
