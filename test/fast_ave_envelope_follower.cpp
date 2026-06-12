/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Contract tests for the fast_ave_envelope_follower: the fast (peak-hold)
// follower's staircase smoothed by a moving average of the same hold
// length. Reads the peak level like the fast follower, but with the
// staircase ripple averaged out — the modulation-use recommendation in
// the docs.
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

TEST_CASE("fast_ave: steady sine reads the peak level")
{
   constexpr float a = 0.8f;
   auto env = q::fast_ave_envelope_follower{period / 2};

   float v = 0.0f;
   for (std::size_t i = 0; i != 12 * period; ++i)
      v = env(std::abs(sine(i, a)));
   CHECK(v == Approx(a).epsilon(0.05));
}

TEST_CASE("fast_ave: the averager turns staircase edges into ramps")
{
   // The fast follower's output is a staircase: a level change lands as
   // an instant jump. The averager spreads that jump over its window —
   // this is the smoothing, pinned deterministically on a step. (A pure
   // steady sine cannot pin it: on a commensurate sample grid the
   // staircase is perfectly flat and both ripples read exactly zero.)
   constexpr std::size_t hold = 100;
   auto fast = q::fast_envelope_follower{hold};
   auto ave  = q::fast_ave_envelope_follower{hold};

   for (std::size_t i = 0; i != 4 * hold; ++i)   // settle both at 0.5
   {
      fast(0.5f);
      ave(0.5f);
   }

   // Step to 1.0: the fast follower jumps within one sample; the
   // averager moves by only one window-slice (0.5 + 0.5/hold).
   CHECK(fast(1.0f) == 1.0f);
   CHECK(ave(1.0f) < 0.55f);

   float am = 0.0f;
   for (std::size_t i = 0; i != hold / 2; ++i)   // half a window later
      am = ave(1.0f);
   CHECK(am > 0.6f);                             // mid-ramp...
   CHECK(am < 0.95f);                            // ...not arrived yet

   float ae = 0.0f;
   for (std::size_t i = 0; i != 2 * hold; ++i)
      ae = ave(1.0f);
   CHECK(ae == Approx(1.0f).epsilon(0.001));     // settled at the top
}

TEST_CASE("fast_ave: no decay sag — a held peak does not droop")
{
   // The property the hold-based family exists for: between resets the
   // reading is HELD, not decayed (a decaying peak follower's sag
   // between waveform peaks chatters anything that ratios envelopes).
   constexpr std::size_t hold = 100;
   auto env = q::fast_ave_envelope_follower{hold};

   float v = 0.0f;
   for (std::size_t i = 0; i != hold; ++i)   // fill the averager
      v = env(1.0f);
   float const top = v;
   for (std::size_t i = 0; i != hold / 2; ++i)
      v = env(0.0f);                          // silence, within the hold
   CHECK(v == Approx(top).epsilon(0.6));      // still well above zero...
   for (std::size_t i = 0; i != 5 * hold; ++i)
      v = env(0.0f);
   CHECK(v < 1e-6f);                          // ...but releases fully
}

TEST_CASE("fast_ave: const getter returns the latest smoothed value")
{
   auto env = q::fast_ave_envelope_follower{std::size_t(100)};
   float v = 0.0f;
   for (int i = 0; i != 50; ++i)
      v = env(0.5f);
   CHECK(env() == v);
}
