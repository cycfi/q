/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Contract tests for the peak_envelope_follower: instantaneous attack
// (tracks any rising sample exactly), exponential release with the
// -2/(sps*release) convention (after `release` seconds of silence, a held
// peak decays to e^-2 of its value).
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/fx/envelope.hpp>
#include <cmath>

namespace q = cycfi::q;
using namespace q::literals;

constexpr float sps = 48000.0f;

TEST_CASE("peak: instantaneous attack — rising input is tracked exactly")
{
   auto env = q::peak_envelope_follower{50_ms, sps};
   for (int i = 0; i != 100; ++i)
   {
      float s = float(i) / 100;     // rising ramp
      CHECK(env(s) == s);           // exact: y = s whenever s > y
   }
}

TEST_CASE("peak: exponential release — e^-2 after the release duration")
{
   constexpr float release_s = 0.05f;
   auto env = q::peak_envelope_follower{q::duration{release_s}, sps};

   env(1.0f);                       // set the peak
   float v = 1.0f;
   auto n = std::size_t(release_s * sps);
   for (std::size_t i = 0; i != n; ++i)
      v = env(0.0f);

   // y = r^n with r = exp(-2/(sps*release)) -> e^-2 after n samples
   CHECK(v == Approx(std::exp(-2.0f)).epsilon(0.02));

   // and the decay is monotonic from a peak over silence
   float prev = v;
   for (int i = 0; i != 100; ++i)
   {
      float cur = env(0.0f);
      CHECK(cur <= prev);
      prev = cur;
   }
}

TEST_CASE("peak: steady sine reads the peak, ripple bounded by the release")
{
   constexpr float a = 0.8f;
   constexpr float f0 = 240.0f;     // 200 samples per period
   auto env = q::peak_envelope_follower{100_ms, sps};

   float v = 0.0f, lo = 1.0f, hi = 0.0f;
   for (int i = 0; i != 4800; ++i)
      v = env(a * std::abs(std::sin(2.0f * float(M_PI) * f0 * i / sps)));
   for (int i = 0; i != 400; ++i)   // two periods, settled
   {
      v = env(a * std::abs(std::sin(2.0f * float(M_PI) * f0 * i / sps)));
      lo = std::min(lo, v);
      hi = std::max(hi, v);
   }
   CHECK(hi == Approx(a).epsilon(0.01));
   // sag between peaks over half a period: a * (1 - e^(-2*0.5*P/release))
   // with P/release = 200/4800: about 0.96 a -- pin loosely
   CHECK(lo > 0.9f * a);
}

TEST_CASE("peak: seeding and getter")
{
   auto env = q::peak_envelope_follower{50_ms, sps};
   env = 0.5f;                      // operator=(float) seeds the state
   CHECK(env() == 0.5f);            // const getter
   CHECK(env(0.0f) < 0.5f);         // and it releases from the seed
}

TEST_CASE("peak: release() mutator changes the rate")
{
   auto slow = q::peak_envelope_follower{100_ms, sps};
   auto fast = q::peak_envelope_follower{100_ms, sps};
   fast.release(10_ms, sps);

   slow(1.0f);
   fast(1.0f);
   float vs = 1.0f, vf = 1.0f;
   for (int i = 0; i != 480; ++i)   // 10 ms of silence
   {
      vs = slow(0.0f);
      vf = fast(0.0f);
   }
   CHECK(vf == Approx(std::exp(-2.0f)).epsilon(0.02));    // 1x its release
   CHECK(vs > vf);                                        // slower decay
}
