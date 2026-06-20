/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/fx/lowpass.hpp>
#include <cmath>
#include <random>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr double pi = 3.14159265358979323846;
   constexpr float sps = 48000.0f;

   enum class mode { lp, bp, hp, notch };

   double pick(q::reso_filter const& f, mode m)
   {
      switch (m)
      {
         case mode::lp:    return f.lowpass();
         case mode::bp:    return f.bandpass();
         case mode::hp:    return f.highpass();
         case mode::notch: return f.notch();
      }
      return 0;
   }

   double gain(q::frequency fc, double Q, mode m, double test_hz)
   {
      q::reso_filter f{fc, sps, Q};
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

TEST_CASE("Chamberlin: passband and resonance equal to Q")
{
   auto const Q = q::reso_filter::default_q;
   // Flat passband below cutoff, and a 2-pole magnitude of ~Q at the cutoff.
   CHECK(gain(500_Hz, Q, mode::lp, 50.0) == Approx(1.0).margin(0.04));
   CHECK(gain(500_Hz, Q, mode::lp, 500.0) == Approx(0.707).margin(0.06));
   // Resonance is the true Q (decoupled from cutoff), within the Chamberlin
   // approximation.
   CHECK(gain(500_Hz, 5.0, mode::lp, 500.0) == Approx(5.0).epsilon(0.15));
}

TEST_CASE("Chamberlin: ~12 dB/oct rolloff")
{
   auto const Q = q::reso_filter::default_q;
   double g1 = gain(100_Hz, Q, mode::lp, 800.0);
   double g2 = gain(100_Hz, Q, mode::lp, 1600.0);
   CHECK((g1 / g2) == Approx(4.0).margin(0.6));
}

TEST_CASE("Chamberlin: multimode shape")
{
   auto const Q = q::reso_filter::default_q;
   double bp_fc = gain(1_kHz, 4.0, mode::bp, 1000.0);
   CHECK(bp_fc > gain(1_kHz, 4.0, mode::bp, 250.0));
   CHECK(bp_fc > gain(1_kHz, 4.0, mode::bp, 4000.0));
   CHECK(gain(1_kHz, Q, mode::notch, 1000.0) < 0.15);
   CHECK(gain(1_kHz, Q, mode::hp, 16000.0) > 0.8);
}

TEST_CASE("Chamberlin: cutoff past fs/6 is clamped, not NaN")
{
   q::reso_filter f{15_kHz, sps, 0.707};   // above fs/6 = 8 kHz
   bool finite = true;
   for (std::size_t i = 0; i < 4096; ++i)
   {
      float y = f((i % 64 < 32) ? 1.0f : -1.0f);   // square excitation
      if (!std::isfinite(y)) { finite = false; break; }
   }
   CHECK(finite);
}

TEST_CASE("Chamberlin: stable under fast per-sample cutoff modulation")
{
   for (double Q : {0.707, 10.0})
   {
      q::reso_filter f{1_kHz, sps, Q};
      std::mt19937 rng{99};
      std::uniform_real_distribution<float> noise{-1.0f, 1.0f};
      bool ok = true;
      auto const n = std::size_t(sps * 2);
      for (std::size_t i = 0; i < n; ++i)
      {
         double lfo = 0.5 * (1.0 + std::sin(2.0 * pi * 20.0 * i / sps));
         double fc = 50.0 + lfo * 7000.0;        // stay within the fs/6 range
         f.cutoff(q::frequency{fc}, sps);
         float y = f(noise(rng));
         if (!std::isfinite(y) || std::abs(y) > 1e3f) { ok = false; break; }
      }
      CHECK(ok);
   }
}
