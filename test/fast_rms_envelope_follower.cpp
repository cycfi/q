/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Contract tests for the fast_rms_envelope_follower: a fast level
// DETECTOR computed in the power domain — holds the peak of s², smooths
// the staircase, square root on output. Reads the PEAK level (A for a
// steady sine, not A/√2); crest-dependent by design. The crest contrast
// with the true RMS follower is pinned in true_rms_envelope_follower.cpp.
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

   float square(std::size_t i, float a)
   {
      return sine(i, 1.0f) >= 0.0f ? a : -a;
   }
}

TEST_CASE("fast_rms: steady sine reads its PEAK, not its RMS")
{
   constexpr float a = 0.8f;
   auto env = q::fast_rms_envelope_follower{q::duration{2.0 / f0}, sps};

   float v = 0.0f;
   for (std::size_t i = 0; i != 12 * period; ++i)
      v = env(sine(i, a));
   CHECK(v == Approx(a).epsilon(0.05));            // peak...
   CHECK(v > 1.2f * a / std::sqrt(2.0f));          // ...clearly not RMS
}

TEST_CASE("fast_rms: square wave reads A (peak == RMS for a square)")
{
   constexpr float a = 0.6f;
   auto env = q::fast_rms_envelope_follower{q::duration{2.0 / f0}, sps};
   float v = 0.0f;
   for (std::size_t i = 0; i != 12 * period; ++i)
      v = env(square(i, a));
   CHECK(v == Approx(a).epsilon(0.05));
}

TEST_CASE("fast_rms: peak_square() is the value under the radical")
{
   constexpr float a = 0.8f;
   auto env = q::fast_rms_envelope_follower{q::duration{2.0 / f0}, sps};
   float v = 0.0f;
   for (std::size_t i = 0; i != 12 * period; ++i)
      v = env(sine(i, a));

   CHECK(env.peak_square() == Approx(a * a).epsilon(0.1));
   CHECK(v == Approx(std::sqrt(env.peak_square())).epsilon(0.005));

   // const getter round-trips the latest reading
   CHECK(env() == Approx(v).epsilon(0.001));
}

TEST_CASE("fast_rms: silence floors")
{
   auto env    = q::fast_rms_envelope_follower{q::duration{2.0 / f0}, sps};
   auto env_db = q::fast_rms_envelope_follower_db{q::duration{2.0 / f0}, sps};

   float v = 1.0f;
   q::decibel d = env_db();
   for (std::size_t i = 0; i != 12 * period; ++i)
   {
      v = env(0.0f);
      d = env_db(0.0f);
   }
   // Below the -120 dB mean-square threshold the value is zeroed before
   // the square root; fast_sqrt(0) is a denormal-range residue, not an
   // exact 0 — pin "effectively silent", robust to an exact-0 change.
   CHECK(v < 1e-10f);
   // The _db variant computes lin_to_db(0)/2 here: far below any signal.
   // Pin "at or below the -60 dB floor" only (the exact value is an
   // artifact of the fast log at 0).
   CHECK(d.rep <= -60.0f);
}

TEST_CASE("fast_rms_db: sine reads its peak in dB")
{
   auto env_db = q::fast_rms_envelope_follower_db{q::duration{2.0 / f0}, sps};
   q::decibel d = env_db();
   for (std::size_t i = 0; i != 12 * period; ++i)
      d = env_db(sine(i, 1.0f));
   CHECK(d.rep == Approx(0.0f).margin(0.5));   // peak of a unit sine: 0 dB
}
