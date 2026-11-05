/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/bend_range.hpp>
#include <q/midi/scaling.hpp>

#include <cmath>
#include <cstdint>

namespace q = cycfi::q;
namespace midi1 = q::midi_1_0;
namespace midi2 = q::midi_2_0;
using q::midi::bend_range;

namespace
{
   midi1::pitch_bend bend1(std::uint16_t value)
   {
      return midi1::pitch_bend{0, value};
   }

   midi2::pitch_bend bend2(std::uint32_t value)
   {
      return midi2::pitch_bend{0, 0, value};
   }
}

TEST_CASE("The center is exactly unison")
{
   bend_range r{12.0f, 7.0f};
   CHECK(r.semitones(bend1(8192)) == 0.0f);
   CHECK(r.ratio(bend1(8192)) == 1.0f);
   CHECK(r.semitones(bend2(midi2::pitch_bend::center)) == 0.0f);
   CHECK(r.ratio(bend2(midi2::pitch_bend::center)) == 1.0f);
}

TEST_CASE("Each end reaches its own full depth")
{
   bend_range r{12.0f, 7.0f};
   CHECK(r.semitones(bend1(0)) == -12.0f);
   CHECK(r.semitones(bend1(16383)) == 7.0f);
   CHECK(r.semitones(bend2(0)) == -12.0f);
   CHECK(r.semitones(bend2(0xFFFFFFFFu)) == 7.0f);

   CHECK(r.ratio(bend1(0)) == Approx(0.5f));
   CHECK(r.ratio(bend1(16383)) == Approx(std::exp2(7.0f / 12.0f)));
}

TEST_CASE("One depth bends the same both ways")
{
   bend_range r{2.0f};
   CHECK(r.down == 2.0f);
   CHECK(r.up == 2.0f);
   CHECK(r.semitones(bend1(0)) == -2.0f);
   CHECK(r.semitones(bend1(16383)) == 2.0f);

   bend_range dflt;
   CHECK(dflt.semitones(bend1(16383)) == 2.0f);
}

TEST_CASE("The bend rises with its value")
{
   bend_range r{24.0f, 12.0f};
   auto prev = r.semitones(bend1(0));
   for (std::uint32_t v = 1; v != 16384; ++v)
   {
      auto const s = r.semitones(bend1(std::uint16_t(v)));
      REQUIRE(s > prev);
      prev = s;
   }
}

TEST_CASE("MIDI 1.0 and MIDI 2.0 agree")
{
   bend_range r{12.0f, 12.0f};
   for (std::uint32_t v = 0; v < 16384; v += 7)
   {
      auto const v2 = midi2::scale_up(v, 14, 32);
      CHECK(r.semitones(bend2(v2))
         == Approx(r.semitones(bend1(std::uint16_t(v)))).margin(1e-5));
   }
}

TEST_CASE("Half way is half the depth")
{
   bend_range r{12.0f, 12.0f};
   CHECK(r.semitones(bend1(4096)) == -6.0f);
   CHECK(r.semitones(bend2(0x40000000u)) == -6.0f);
}

TEST_CASE("Vibrato across the middle is as deep up as down")
{
   bend_range r{12.0f, 7.0f, 1.0f};
   CHECK(r.linear == 1.0f);

   // The middle runs at the mean rate, 9.5 semitones a half, out to
   // 1 semitone either way.
   auto const w = 1.0 / 9.5;
   for (int d = 0; d <= int(w * 8191); d += 10)
   {
      auto const up = r.semitones(bend1(std::uint16_t(8192 + d)));
      auto const dn = r.semitones(bend1(std::uint16_t(8192 - d)));
      REQUIRE(up == Approx(9.5 * d / 8191.0).margin(1e-5));
      REQUIRE(dn == Approx(-9.5 * d / 8192.0).margin(1e-5));
   }
}

TEST_CASE("Three straight segments: down, middle and up")
{
   bend_range r{12.0f, 7.0f, 1.0f};
   auto at = [&](double x)
   {
      auto const d = x < 0 ? x * 8192 : x * 8191;
      return r.semitones(bend1(std::uint16_t(std::lround(8192 + d))));
   };

   // The breaks: 1 semitone either way, 1/9.5 of each half's travel.
   auto const w = 1.0 / 9.5;
   CHECK(at(w) == Approx(1.0f).margin(1e-3));
   CHECK(at(-w) == Approx(-1.0f).margin(1e-3));

   // Each segment is straight: its midpoint is the mean of its ends.
   CHECK(at((-1 - w) / 2) == Approx((at(-1) + at(-w)) / 2).margin(2e-3));
   CHECK(at((1 + w) / 2) == Approx((at(1) + at(w)) / 2).margin(2e-3));
   CHECK(at(-1) == -12.0f);
   CHECK(at(1) == 7.0f);
}

TEST_CASE("A side of depth 0 leaves no middle")
{
   bend_range r{12.0f, 0.0f, 1.0f};
   CHECK(r.semitones(bend1(16383)) == 0.0f);
   CHECK(r.semitones(bend1(4096)) == -6.0f);
   CHECK(r.semitones(bend1(0)) == -12.0f);
}

TEST_CASE("The middle is limited to the smaller depth")
{
   bend_range r{12.0f, 1.0f, 3.0f};
   CHECK(r.semitones(bend1(0)) == -12.0f);
   CHECK(r.semitones(bend1(16383)) == 1.0f);
   CHECK(r.semitones(bend1(4096)) == Approx(-5.5f).margin(1e-4));
}
