/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/utility/audio_clock.hpp>
#include <q/support/literals.hpp>

namespace q = cycfi::q;
using namespace q::literals;

TEST_CASE("audio_clock: counts the frames processed")
{
   q::audio_clock clock{48000};
   CHECK(clock.now() == 0);

   clock.advance(64);
   clock.advance(128);
   CHECK(clock.now() == 192);
}

TEST_CASE("audio_clock: a duration in frames, at the sample rate")
{
   q::audio_clock clock{48000};
   CHECK(clock.ticks(1_s) == 48000);
   CHECK(clock.ticks(10_ms) == 480);
   CHECK(clock.ticks(0_s) == 0);

   clock.sps(44100);
   CHECK(clock.ticks(1_s) == 44100);
   CHECK(clock.ticks(10_ms) == 441);
}
