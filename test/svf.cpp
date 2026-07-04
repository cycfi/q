/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/fx/svf.hpp>
#include <cmath>
#include <random>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr double pi = 3.14159265358979323846;
   constexpr float sps = 48000.0f;

   enum class mode { lp, bp, hp, notch, peak, ap };

   double pick(q::svf const& f, mode m)
   {
      switch (m)
      {
         case mode::lp:    return f.lowpass();
         case mode::bp:    return f.bandpass();
         case mode::hp:    return f.highpass();
         case mode::notch: return f.notch();
         case mode::peak:  return f.peak();
         case mode::ap:    return f.allpass();
      }
      return 0;
   }

   // Steady-state magnitude of mode m at test_hz for an svf tuned to (fc, Q).
   // Excite with a unit sine, discard the transient, compare output to input
   // RMS.
   double gain(q::frequency fc, double Q, mode m, double test_hz)
   {
      q::svf f{fc, sps, Q};
      auto const warm = std::size_t(sps * 0.5);
      auto const meas = std::size_t(sps * 0.5);
      double const w = 2.0 * pi * test_hz / sps;
      double in2 = 0, out2 = 0;
      for (std::size_t i = 0; i < warm + meas; ++i)
      {
         double x = std::sin(w * i);
         f(float(x));
         double y = pick(f, m);
         if (i >= warm) { in2 += x * x; out2 += y * y; }
      }
      return std::sqrt(out2 / in2);
   }
}

TEST_CASE("Lowpass: exact cutoff and Butterworth flatness")
{
   auto const Q = q::svf::default_q;     // 1/sqrt(2)

   // Flat passband well below cutoff.
   CHECK(gain(1_kHz, Q, mode::lp, 50.0) == Approx(1.0).margin(0.02));

   // At the cutoff, a 2-pole response has magnitude Q; Butterworth Q gives
   // exactly -3 dB. This is the "exact cutoff" claim.
   CHECK(gain(1_kHz, Q, mode::lp, 1000.0) == Approx(0.70710678).margin(0.02));

   // The prewarp keeps the cutoff exact at a high fc too (the old reso_filter
   // broke and divided by zero up here).
   CHECK(gain(10_kHz, Q, mode::lp, 10000.0) == Approx(0.70710678).margin(0.03));
}

TEST_CASE("Lowpass: 12 dB/oct (two-pole) rolloff")
{
   auto const Q = q::svf::default_q;
   // An octave step well above cutoff should drop the gain by ~4x (12 dB).
   // Measure well below Nyquist (fc = 200, octave 1600 -> 3200, both << 24 kHz)
   // so the bilinear prewarp does not steepen the slope; near Nyquist a TPT
   // filter correctly rolls off faster than the analog prototype.
   double g2 = gain(200_Hz, Q, mode::lp, 1600.0);
   double g4 = gain(200_Hz, Q, mode::lp, 3200.0);
   CHECK((g2 / g4) == Approx(4.0).margin(0.4));
}

TEST_CASE("Resonance equals Q: lowpass peak height")
{
   // For high Q the lowpass peaks at ~Q near the cutoff.
   CHECK(gain(1_kHz, 5.0, mode::lp, 1000.0) == Approx(5.0).epsilon(0.10));
   CHECK(gain(1_kHz, 10.0, mode::lp, 1000.0) == Approx(10.0).epsilon(0.12));
}

TEST_CASE("Highpass mirrors lowpass")
{
   auto const Q = q::svf::default_q;
   CHECK(gain(1_kHz, Q, mode::hp, 16000.0) == Approx(1.0).margin(0.03));
   CHECK(gain(1_kHz, Q, mode::hp, 1000.0) == Approx(0.70710678).margin(0.03));
   CHECK(gain(1_kHz, Q, mode::hp, 50.0) < 0.01);
}

TEST_CASE("Bandpass peaks at the cutoff")
{
   double at_fc = gain(1_kHz, 4.0, mode::bp, 1000.0);
   CHECK(at_fc > gain(1_kHz, 4.0, mode::bp, 250.0));
   CHECK(at_fc > gain(1_kHz, 4.0, mode::bp, 4000.0));
}

TEST_CASE("Notch nulls at the cutoff, passes elsewhere")
{
   auto const Q = q::svf::default_q;
   CHECK(gain(1_kHz, Q, mode::notch, 1000.0) < 0.05);
   CHECK(gain(1_kHz, Q, mode::notch, 50.0) == Approx(1.0).margin(0.03));
   CHECK(gain(1_kHz, Q, mode::notch, 16000.0) == Approx(1.0).margin(0.05));
}

TEST_CASE("Allpass has flat unity magnitude")
{
   auto const Q = q::svf::default_q;
   CHECK(gain(1_kHz, Q, mode::ap, 100.0) == Approx(1.0).margin(0.03));
   CHECK(gain(1_kHz, Q, mode::ap, 1000.0) == Approx(1.0).margin(0.03));
   CHECK(gain(1_kHz, Q, mode::ap, 8000.0) == Approx(1.0).margin(0.03));
}

TEST_CASE("Stable and bounded under fast per-sample cutoff modulation")
{
   for (double Q : {0.707, 20.0})
   {
      q::svf f{1_kHz, sps, Q};
      std::mt19937 rng{12345};
      std::uniform_real_distribution<float> noise{-1.0f, 1.0f};
      bool ok = true;
      auto const n = std::size_t(sps * 2);
      for (std::size_t i = 0; i < n; ++i)
      {
         // Sweep the cutoff across most of the band every few ms.
         double lfo = 0.5 * (1.0 + std::sin(2.0 * pi * 30.0 * i / sps));
         double fc = 50.0 + lfo * 19000.0;
         f.cutoff(q::frequency{fc}, sps);
         float y = f(noise(rng));
         if (!std::isfinite(y) || std::abs(y) > 1e3f)
            { ok = false; break; }
      }
      CHECK(ok);
   }
}

TEST_CASE("Self-oscillation (k = 0) stays bounded")
{
   q::svf f{1_kHz, sps, 0.707};
   f.normalized_resonance(1.0f);          // k = 0, undamped resonator
   float peak = 0.0f;
   bool finite = true;
   for (std::size_t i = 0; i < std::size_t(sps * 4); ++i)
   {
      float x = (i == 0) ? 1.0f : 0.0f;   // single impulse, then silence
      float y = f(x);
      if (!std::isfinite(y)) { finite = false; break; }
      peak = std::max(peak, std::abs(y));
   }
   CHECK(finite);
   CHECK(peak < 5.0f);                     // rings forever, but does not grow
}
