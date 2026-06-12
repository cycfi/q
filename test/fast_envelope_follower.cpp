/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Contract tests for the basic_fast_envelope_follower<div>: div+1 peak
// slots, one reset round-robin every `hold` ticks, output = max slot.
// Instant attack, hold-then-drop release (no exponential sag), staircase
// output. Per the header docs, hold should be >= 1/div of the period of
// the lowest tracked frequency.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/fx/envelope.hpp>
#include <cmath>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr float sps = 48000.0f;
   constexpr float f0  = 240.0f;             // 200 samples per period
   constexpr std::size_t period = 200;

   float sine(std::size_t i, float a)
   {
      return a * std::sin(2.0f * float(M_PI) * f0 * float(i) / sps);
   }
}

TEST_CASE("fast: instantaneous attack")
{
   auto env = q::fast_envelope_follower{std::size_t(100)};
   CHECK(env(0.7f) >= 0.7f);        // the very first sample registers
   CHECK(env(0.9f) >= 0.9f);        // any rise registers immediately
}

TEST_CASE("fast: steady sine at hold = period/2 reads the peak, low ripple")
{
   constexpr float a = 0.8f;
   auto env = q::fast_envelope_follower{period / 2};   // div = 2

   float v = 0.0f;
   for (std::size_t i = 0; i != 8 * period; ++i)       // settle
      v = env(std::abs(sine(i, a)));

   float lo = 1.0f, hi = 0.0f;
   for (std::size_t i = 0; i != 4 * period; ++i)
   {
      v = env(std::abs(sine(i, a)));
      lo = std::min(lo, v);
      hi = std::max(hi, v);
   }
   CHECK(hi == Approx(a).epsilon(0.001));   // reads the true peak
   CHECK(lo > 0.9f * a);                    // staircase ripple is small
}

TEST_CASE("fast: hold-then-drop — no sag while held, gone within div+1 holds")
{
   constexpr std::size_t hold = 100;
   auto env = q::fast_envelope_follower{hold};   // div = 2: 3 slots

   env(1.0f);                       // a single full-scale impulse

   // Held EXACTLY (no exponential sag) for at least one hold period.
   float v = 0.0f;
   for (std::size_t i = 0; i != hold; ++i)
      v = env(0.0f);
   CHECK(v == 1.0f);

   // All slots reset within (div+1) hold periods: exactly zero after,
   // (small slack for the tick counter's reset phase).
   for (std::size_t i = 0; i != 3 * hold; ++i)
      v = env(0.0f);
   CHECK(v == 0.0f);
}

TEST_CASE("fast: div=1 variant drops within 2 holds")
{
   constexpr std::size_t hold = 100;
   auto env = q::basic_fast_envelope_follower<1>{hold};   // 2 slots

   env(1.0f);
   float v = 0.0f;
   for (std::size_t i = 0; i != 3 * hold; ++i)
      v = env(0.0f);
   CHECK(v == 0.0f);
}

TEST_CASE("fast: the documented minimum hold — flat at the rule, sag below it")
{
   // The guaranteed lookback is div*hold: the age of the oldest
   // surviving slot, at its worst right after a reset. The docs require
   // hold >= P/div precisely so that this window always spans one full
   // period of the tracked signal — and therefore always contains a
   // true peak. The rectified sine here repeats every 100 samples
   // (half the 200-sample period), so the minimum hold is 50.
   constexpr float a = 0.8f;
   auto flat = q::fast_envelope_follower{std::size_t(60)};  // above the rule
   auto sag  = q::fast_envelope_follower{std::size_t(5)};   // well below

   float vf = 0.0f, vs = 0.0f;
   for (std::size_t i = 0; i != 8 * period; ++i)            // settle
   {
      vf = flat(std::abs(sine(i, a)));
      vs = sag(std::abs(sine(i, a)));
   }
   float flo = 1.0f, fhi = 0.0f, slo = 1.0f, shi = 0.0f;
   for (std::size_t i = 0; i != 4 * period; ++i)
   {
      vf = flat(std::abs(sine(i, a)));
      vs = sag(std::abs(sine(i, a)));
      flo = std::min(flo, vf); fhi = std::max(fhi, vf);
      slo = std::min(slo, vs); shi = std::max(shi, vs);
   }

   // Conforming: every window holds a true peak — reads the peak, flat.
   CHECK(fhi == Approx(a).epsilon(0.001));
   CHECK(flo > 0.99f * a);

   // Violating: the crests are still caught (instant attack), but
   // between them the window holds no peak — the output leaks the
   // waveform's troughs and ripples at the signal's period rate.
   // (hold = 5: lookback 12..18 samples, so at every trough the best
   // surviving sample is at most a*sin(2*pi*18/200) ~ 0.54a.)
   CHECK(shi == Approx(a).epsilon(0.001));
   CHECK(slo < 0.6f * a);
}

TEST_CASE("fast: const getter returns the latest peak")
{
   auto env = q::fast_envelope_follower{std::size_t(100)};
   env(0.6f);
   CHECK(env() == 0.6f);
}
