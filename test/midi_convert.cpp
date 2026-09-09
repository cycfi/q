/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q_io/detail/midi_convert.hpp>
#include <q/midi/messages.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;

namespace
{
   // The device layer hands over bytes; this is what q::midi_1_0 expects to
   // read them as.
   bool convert(std::vector<std::uint8_t> const& bytes, midi::raw_message& out)
   {
      return q::detail::to_raw_message(
         std::span<std::uint8_t const>{bytes.data(), bytes.size()}, out);
   }
}

TEST_CASE("A three byte message packs little-endian")
{
   midi::raw_message out;
   REQUIRE(convert({0x90, 0x3C, 0x40}, out));

   // Status in the low byte, then data-1, then data-2.
   CHECK(out.data == 0x00403C90);

   midi::note_on const msg{out};
   CHECK(msg.channel() == 0);
   CHECK(msg.key() == 60);
   CHECK(msg.velocity() == 64);
}

TEST_CASE("The channel survives the trip")
{
   midi::raw_message out;
   REQUIRE(convert({0x9F, 0x45, 0x7F}, out));

   midi::note_on const msg{out};
   CHECK(msg.channel() == 15);
   CHECK(msg.key() == 69);
   CHECK(msg.velocity() == 127);
}

TEST_CASE("A two byte message leaves the third byte zero")
{
   midi::raw_message out;
   REQUIRE(convert({0xC3, 0x07}, out));
   CHECK(out.data == 0x000007C3);

   midi::program_change const msg{out};
   CHECK(msg.channel() == 3);
}

TEST_CASE("A one byte message is just the status")
{
   midi::raw_message out;
   REQUIRE(convert({0xF8}, out));
   CHECK(out.data == 0x000000F8);
}

TEST_CASE("Nothing is not a message")
{
   midi::raw_message out{0xDEADBEEF};
   CHECK_FALSE(convert({}, out));

   // A rejected conversion leaves the destination alone.
   CHECK(out.data == 0xDEADBEEF);
}

TEST_CASE("A sysex is too long to pack, and says so")
{
   // Sysex has no place in a 24 bit word. Phase 2 gives it a home; until
   // then the device layer must refuse it rather than truncate it.
   midi::raw_message out{0};
   CHECK_FALSE(convert({0xF0, 0x7E, 0x00, 0x06, 0x01, 0xF7}, out));
   CHECK(out.data == 0);
}

TEST_CASE("Every channel voice status round trips")
{
   // The status bytes q::midi_1_0 dispatches on, each with plausible data.
   struct
   {
      std::uint8_t status;
      std::uint8_t d1;
      std::uint8_t d2;
   }
   const cases[] =
   {
      {0x80, 0x3C, 0x40},     // note off
      {0x90, 0x3C, 0x40},     // note on
      {0xA0, 0x3C, 0x20},     // poly aftertouch
      {0xB0, 0x07, 0x64},     // control change
      {0xD0, 0x30, 0x00},     // channel aftertouch
      {0xE0, 0x00, 0x40},     // pitch bend
   };

   for (auto const& c : cases)
   {
      midi::raw_message out;
      REQUIRE(convert({c.status, c.d1, c.d2}, out));
      CHECK((out.data & 0xFF) == c.status);
      CHECK(((out.data >> 8) & 0xFF) == c.d1);
      CHECK(((out.data >> 16) & 0xFF) == c.d2);
   }
}
