/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Contract tests for the true_rms_envelope_follower: it measures signal
// POWER (root-mean-square level), independent of the waveform's crest
// factor — the property that distinguishes it from the fast follower
// family (incl. fast_rms_envelope_follower, which holds the peak of s²
// and reads the PEAK level).
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
   constexpr float f0  = 240.0f;             // exactly 200 samples/period
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

TEST_CASE("true_rms: steady sine reads A over root 2, ripple-free")
{
   constexpr float a = 0.8f;
   auto env = q::true_rms_envelope_follower{4 * period};   // 4 full cycles

   // Settle for well over one window
   std::size_t i = 0;
   for (; i != 8 * period; ++i)
      env(sine(i, a));

   // Per-sample readings over the next 4 periods: correct level AND
   // flat (an integer-period window nulls the s² ripple).
   float lo = 10.0f, hi = 0.0f;
   for (auto end = i + 4 * period; i != end; ++i)
   {
      auto v = env(sine(i, a));
      lo = std::min(lo, v);
      hi = std::max(hi, v);
   }
   float const rms = a / std::sqrt(2.0f);
   CHECK(lo == Approx(rms).epsilon(0.005));
   CHECK(hi == Approx(rms).epsilon(0.005));
   CHECK((hi - lo) < 0.001f * rms);
}

TEST_CASE("true_rms: square wave reads A")
{
   constexpr float a = 0.6f;
   auto env = q::true_rms_envelope_follower{4 * period};
   float v = 0.0f;
   for (std::size_t i = 0; i != 8 * period; ++i)
      v = env(square(i, a));
   CHECK(v == Approx(a).epsilon(0.005));
}

TEST_CASE("true_rms: crest independence — equal power reads equal")
{
   // A sine and a square scaled to the SAME RMS level. The true RMS
   // follower reads them identically; the fast_rms_envelope_follower —
   // a peak detector in the squared domain — reads the sine ~3 dB
   // higher (its peak). This is the executable form of the distinction.
   constexpr float rms     = 0.5f;
   constexpr float sine_a  = rms * 1.41421356f;   // sine RMS = A/√2
   constexpr float sq_a    = rms;                 // square RMS = A

   auto true_sine = q::true_rms_envelope_follower{4 * period};
   auto true_sq   = q::true_rms_envelope_follower{4 * period};
   auto fast_sine = q::fast_rms_envelope_follower{q::duration{4.0 / f0}, sps};
   auto fast_sq   = q::fast_rms_envelope_follower{q::duration{4.0 / f0}, sps};

   float ts = 0, tq = 0, fs = 0, fq = 0;
   for (std::size_t i = 0; i != 12 * period; ++i)
   {
      ts = true_sine(sine(i, sine_a));
      tq = true_sq(square(i, sq_a));
      fs = fast_sine(sine(i, sine_a));
      fq = fast_sq(square(i, sq_a));
   }

   // True RMS: both read the common power level.
   CHECK(ts == Approx(rms).epsilon(0.005));
   CHECK(tq == Approx(rms).epsilon(0.005));

   // fast_rms: reads peaks — the same-power sine reads ~√2 higher.
   CHECK(fs == Approx(sine_a).epsilon(0.05));
   CHECK(fq == Approx(sq_a).epsilon(0.05));
   CHECK(fs > 1.3f * fq);

   // peak_square() is the value under fast_rms's radical — the
   // counterpart of mean_square(): held peak of s², smoothed.
   CHECK(fast_sine.peak_square() == Approx(sine_a * sine_a).epsilon(0.1));
   CHECK(fs == Approx(std::sqrt(fast_sine.peak_square())).epsilon(0.005));
}

TEST_CASE("true_rms: mean_square exposes the squared domain")
{
   constexpr float a = 0.8f;
   auto env = q::true_rms_envelope_follower{4 * period};
   float v = 0.0f;
   for (std::size_t i = 0; i != 8 * period; ++i)
      v = env(sine(i, a));

   CHECK(env.mean_square() == Approx(a * a / 2.0f).epsilon(0.005));
   CHECK(v == Approx(std::sqrt(env.mean_square())).epsilon(0.005));
}

TEST_CASE("true_rms: silence floors at zero; db variant at -60 dB")
{
   auto env    = q::true_rms_envelope_follower{4 * period};
   auto env_db = q::true_rms_envelope_follower_db{4 * period};

   float v = 1.0f;
   q::decibel d = env_db();
   for (std::size_t i = 0; i != 8 * period; ++i)
   {
      v = env(0.0f);
      d = env_db(0.0f);
   }
   CHECK(v == 0.0f);
   CHECK(d.rep == Approx(-60.0f).margin(0.1));
}

TEST_CASE("true_rms_db: sine reads 3 dB below its peak")
{
   auto env_db = q::true_rms_envelope_follower_db{4 * period};
   q::decibel d = env_db();
   for (std::size_t i = 0; i != 8 * period; ++i)
      d = env_db(sine(i, 1.0f));
   CHECK(d.rep == Approx(-3.01f).margin(0.1));
}
