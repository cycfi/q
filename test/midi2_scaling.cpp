/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Every number here is taken from MIDI 2.0 Bit Scaling and Resolution,
// version 1.0.1, May 23 2023 (MMA/AMEI M2-115-U), Tables 5 to 10. The
// tests are the specification's own worked examples.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/scaling.hpp>

#include <cstdint>

namespace q = cycfi::q;
namespace midi2 = q::midi_2_0;

// 3 Min-Center-Max scaling ///////////////////////////////////////////////////

TEST_CASE("3.2 The three anchors: minimum, centre and maximum hold")
{
   // "Minimum/Lowest value is scaled to Minimum/Lowest ... Maximum/Highest
   // value is scaled to Maximum/Highest ... Center Value (rounded up)
   // scales to Center Value"
   CHECK(midi2::scale_up(0, 7, 16) == 0);
   CHECK(midi2::scale_up(64, 7, 16) == 0x8000);
   CHECK(midi2::scale_up(127, 7, 16) == 0xFFFF);

   CHECK(midi2::scale_up(0, 7, 32) == 0);
   CHECK(midi2::scale_up(64, 7, 32) == 0x80000000u);
   CHECK(midi2::scale_up(127, 7, 32) == 0xFFFFFFFFu);
}

TEST_CASE("Table 5 Upscale 7 to 16 bits")
{
   struct { std::uint32_t in, out; } const rows[] =
   {
      {0, 0x0000}, {5, 0x0A00}, {30, 0x3C00}, {32, 0x4000}, {64, 0x8000}
    , {70, 0x8C30}, {96, 0xC104}, {120, 0xF1C7}, {127, 0xFFFF}
   };
   for (auto const& r : rows)
      CHECK(midi2::scale_up(r.in, 7, 16) == r.out);
}

TEST_CASE("Table 6 Upscale 7 to 32 bits")
{
   struct { std::uint32_t in, out; } const rows[] =
   {
      {0, 0x00000000u}, {5, 0x0A000000u}, {30, 0x3C000000u}
    , {32, 0x40000000u}, {64, 0x80000000u}, {70, 0x8C30C30Cu}
    , {96, 0xC1041041u}, {120, 0xF1C71C71u}, {127, 0xFFFFFFFFu}
   };
   for (auto const& r : rows)
      CHECK(midi2::scale_up(r.in, 7, 32) == r.out);
}

TEST_CASE("Table 7 Upscale 16 to 32 bits")
{
   struct { std::uint32_t in, out; } const rows[] =
   {
      {0, 0x00000000u}, {5, 0x00050000u}, {30, 0x001E0000u}
    , {16384, 0x40000000u}, {32768, 0x80000000u}, {40000, 0x9C403880u}
    , {49152, 0xC0008001u}, {65000, 0xFDE8FBD1u}, {65535, 0xFFFFFFFFu}
   };
   for (auto const& r : rows)
      CHECK(midi2::scale_up(r.in, 16, 32) == r.out);
}

TEST_CASE("Table 8 Downscale 16 to 7 bits is a shift")
{
   // 3.4.1: "simple bit shift"
   struct { std::uint32_t in, out; } const rows[] =
   {
      {5120, 10}, {32768, 64}, {44730, 87}, {65535, 127}
   };
   for (auto const& r : rows)
      CHECK(midi2::scale_down(r.in, 16, 7) == r.out);
}

TEST_CASE("3.2 Scaling down a previously upscaled value yields the original")
{
   for (std::uint32_t v = 0; v != 128; ++v)
   {
      CHECK(midi2::scale_down(midi2::scale_up(v, 7, 16), 16, 7) == v);
      CHECK(midi2::scale_down(midi2::scale_up(v, 7, 32), 32, 7) == v);
   }
   for (std::uint32_t v = 0; v < 16384; v += 7)
      CHECK(midi2::scale_down(midi2::scale_up(v, 14, 32), 32, 14) == v);
}

TEST_CASE("Upscaling 14 bit pitch bend keeps the centre centred")
{
   // The bend most in need of this: 8192 must land exactly on 0x80000000,
   // or every idle wheel would detune.
   CHECK(midi2::scale_up(8192, 14, 32) == 0x80000000u);
   CHECK(midi2::scale_up(0, 14, 32) == 0);
   CHECK(midi2::scale_up(16383, 14, 32) == 0xFFFFFFFFu);
}

TEST_CASE("Same width is the identity")
{
   CHECK(midi2::scale_up(100, 7, 7) == 100);
   CHECK(midi2::scale_down(100, 7, 7) == 100);
}

// 4 Zero-extension scaling with rounding /////////////////////////////////////

TEST_CASE("4.2 Zero extension: the maximum is a shift, not a fill")
{
   // "Maximum/Highest value is upscaled using bitshift only. For example,
   // a 7-bit value of 127 is translated to a 16-bit value of 65024."
   CHECK(midi2::zero_extend_up(127, 7, 16) == 65024);
   CHECK(midi2::zero_extend_up(64, 7, 16) == 0x8000);
   CHECK(midi2::zero_extend_up(0, 7, 16) == 0);
}

TEST_CASE("Table 9 Zero extension up, 7 to 16 bits")
{
   struct { std::uint32_t in, out; } const rows[] =
   {
      {10, 5120}, {64, 32768}, {87, 44544}
   };
   for (auto const& r : rows)
      CHECK(midi2::zero_extend_up(r.in, 7, 16) == r.out);
}

TEST_CASE("Table 10 Zero extension down rounds, 16 to 7 bits")
{
   // 4.4: "first adding half of the scaled range before shifting down.
   // Then, if the shifted value exceeds the bounds ... clamp it"
   struct { std::uint32_t in, out; } const rows[] =
   {
      {5120, 10}, {5631, 11}, {32768, 64}, {44544, 87}, {44730, 87}
   };
   for (auto const& r : rows)
      CHECK(midi2::zero_extend_down(r.in, 16, 7) == r.out);
}

TEST_CASE("4.4.1 Rounding at the top clamps rather than wraps")
{
   // 65535 + half range shifts to 128, one past a 7 bit maximum.
   CHECK(midi2::zero_extend_down(65535, 16, 7) == 127);
}

TEST_CASE("4.2 Zero extension also round trips")
{
   for (std::uint32_t v = 0; v != 128; ++v)
   {
      auto const up = midi2::zero_extend_up(v, 7, 16);
      CHECK(midi2::zero_extend_down(up, 16, 7) == v);
   }
}

// 3.1 and 4.1: which scheme a message uses ///////////////////////////////////

TEST_CASE("3.1 and 4.1 Registered controllers split on the index")
{
   // "Zero-Extension Scaling on all Registered Controller (RPN) Messages
   // where the index (RPN LSB) is 0-31", and Min-Center-Max on the rest.
   // Index 0 is pitch bend sensitivity, a count of semitones: a value that
   // must not be stretched.
   CHECK(midi2::registered_uses_zero_extension(0));
   CHECK(midi2::registered_uses_zero_extension(31));
   CHECK_FALSE(midi2::registered_uses_zero_extension(32));
   CHECK_FALSE(midi2::registered_uses_zero_extension(127));
}
