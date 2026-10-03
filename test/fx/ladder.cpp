/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/fx/ladder.hpp>
#include <cmath>
#include <random>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr double pi = 3.14159265358979323846;
   constexpr float sps = 48000.0f;

   // Steady-state magnitude of the ladder lowpass at test_hz for (fc, reso).
   double gain(q::frequency fc, float reso, double test_hz)
   {
      q::moog_ladder f{fc, sps, reso};
      auto const warm = std::size_t(sps * 0.5);
      auto const meas = std::size_t(sps * 0.5);
      double const w = 2.0 * pi * test_hz / sps;
      double in2 = 0, out2 = 0;
      for (std::size_t i = 0; i < warm + meas; ++i)
      {
         double x = std::sin(w * i);
         double y = f(float(x));
         if (i >= warm) { in2 += x * x; out2 += y * y; }
      }
      return std::sqrt(out2 / in2);
   }
}

TEST_CASE("Ladder: flat passband, -12 dB at the per-pole corner")
{
   // No resonance: unity well below cutoff.
   CHECK(gain(1_kHz, 0.0f, 50.0) == Approx(1.0).margin(0.03));

   // Four one-poles each at their -3 dB corner give (1/sqrt2)^4 = 0.25 at fc.
   CHECK(gain(1_kHz, 0.0f, 1000.0) == Approx(0.25).margin(0.03));
}

TEST_CASE("Ladder: 24 dB/oct (four-pole) rolloff")
{
   // Octave step well above cutoff and below Nyquist drops gain ~16x.
   double g1 = gain(200_Hz, 0.0f, 1600.0);
   double g2 = gain(200_Hz, 0.0f, 3200.0);
   CHECK((g1 / g2) == Approx(16.0).margin(2.5));
}

TEST_CASE("Ladder: resonance lifts a peak at the cutoff")
{
   double flat = gain(1_kHz, 0.0f, 1000.0);
   double reso = gain(1_kHz, 0.9f, 1000.0);
   CHECK(reso > flat);
   CHECK(reso > 1.0);                   // a real resonant boost above unity
}

TEST_CASE("Ladder: self-oscillation at r = 1 stays bounded")
{
   q::moog_ladder f{1_kHz, sps, 1.0f};   // k = 4, self-oscillation threshold
   float peak = 0.0f;
   double late2 = 0.0;
   bool finite = true;
   auto const n = std::size_t(sps * 4);
   for (std::size_t i = 0; i < n; ++i)
   {
      float x = (i == 0) ? 1.0f : 0.0f;  // impulse, then silence
      float y = f(x);
      if (!std::isfinite(y)) { finite = false; break; }
      peak = std::max(peak, std::abs(y));
      if (i >= n - std::size_t(sps)) late2 += double(y) * y;   // last second
   }
   CHECK(finite);
   CHECK(peak < 10.0f);                              // does not blow up
   CHECK(std::sqrt(late2 / sps) > 1e-3);             // still ringing
}

TEST_CASE("Ladder: stable under fast per-sample cutoff modulation")
{
   for (bool nl : {false, true})
   {
      q::moog_ladder f{1_kHz, sps, 0.9f, nl};
      std::mt19937 rng{2024};
      std::uniform_real_distribution<float> noise{-1.0f, 1.0f};
      bool ok = true;
      auto const n = std::size_t(sps * 2);
      for (std::size_t i = 0; i < n; ++i)
      {
         double lfo = 0.5 * (1.0 + std::sin(2.0 * pi * 25.0 * i / sps));
         double fc = 50.0 + lfo * 18000.0;
         f.cutoff(q::frequency{fc}, sps);
         float y = f(noise(rng));
         if (!std::isfinite(y) || std::abs(y) > 1e3f) { ok = false; break; }
      }
      CHECK(ok);
   }
}

TEST_CASE("Ladder: nonlinear mode is bounded when driven hard")
{
   q::moog_ladder f{800_Hz, sps, 1.0f, true};   // self-osc + nonlinear
   float peak = 0.0f;
   bool finite = true;
   double const w = 2.0 * pi * 800.0 / sps;
   for (std::size_t i = 0; i < std::size_t(sps * 2); ++i)
   {
      float y = f(4.0f * std::sin(w * i));       // overdrive the input
      if (!std::isfinite(y)) { finite = false; break; }
      peak = std::max(peak, std::abs(y));
   }
   CHECK(finite);
   CHECK(peak < 50.0f);
}

TEST_CASE("Ladder: the pole count is a parameter")
{
   // The stages and the cell are independent axes. Four poles is what the
   // two shipped filters use; the base takes any count.
   using two = q::basic_ladder<q::detail::moog_cell, 2>;
   using four = q::basic_ladder<q::detail::moog_cell, 4>;
   using six = q::basic_ladder<q::detail::ota_cell, 6>;

   CHECK(two::poles == 2);
   CHECK(four::poles == 4);
   CHECK(six::poles == 6);

   // Four poles reach self-oscillation at k = 4, which is what the filter
   // used before the count was a parameter, so nothing moved.
   CHECK(four::k_limit() == Approx(4.0f));
   CHECK(q::moog_ladder::k_limit() == Approx(4.0f));

   // sec(pi/N)^N elsewhere: 8 at three poles, 2.37 at six.
   CHECK(q::basic_ladder<q::detail::moog_cell, 3>::k_limit()
      == Approx(8.0f));
   CHECK(six::k_limit() == Approx(2.370f).margin(0.005f));

   // Two poles never reach 180 degrees, so r is a plain feedback amount.
   CHECK(two::k_limit() == Approx(4.0f));
}

TEST_CASE("Ladder: each pole count gives its own prototype")
{
   // With no resonance the ladder is N one-poles, so its magnitude is the
   // prototype's at the prewarped ratio: |H| = (1 + r^2)^(-N/2) where
   // r = tan(pi f/fs) / tan(pi fc/fs). That is the check to make near
   // Nyquist, where the warping steepens the response well past the -6N dB
   // an octave asymptote.
   auto measure = [](auto&& filt, double f)
   {
      double const step = 2.0 * pi * f / sps;
      float p = 0.0f;
      for (std::size_t i = 0; i != 20000; ++i)
      {
         float y = filt(float(std::sin(step * i)));
         if (i > 10000)
            p = std::max(p, std::abs(y));
      }
      filt = 0.0f;
      return 20.0 * std::log10(std::max(p, 1e-9f));
   };

   auto prototype = [](double f, double fc, int n)
   {
      auto r = std::tan(pi * f / sps) / std::tan(pi * fc / sps);
      return -10.0 * n * std::log10(1.0 + r * r);
   };

   q::basic_ladder<q::detail::moog_cell, 2> two{1_kHz, sps};
   q::basic_ladder<q::detail::moog_cell, 4> four{1_kHz, sps};
   q::basic_ladder<q::detail::ota_cell, 6> six{1_kHz, sps};

   for (double f : {500.0, 1000.0, 2000.0, 4000.0})
   {
      CHECK(measure(two, f) == Approx(prototype(f, 1000.0, 2)).margin(0.6));
      CHECK(measure(four, f) == Approx(prototype(f, 1000.0, 4)).margin(0.8));
      CHECK(measure(six, f) == Approx(prototype(f, 1000.0, 6)).margin(1.2));
   }
}

TEST_CASE("Ladder: three and six poles self-oscillate at r = 1")
{
   auto rings = [](auto&& filt)
   {
      filt(1.0f);                            // one impulse, then silence
      float peak = 0.0f;
      for (std::size_t i = 0; i != 20000; ++i)
      {
         float y = filt(0.0f);
         if (i > 10000)
            peak = std::max(peak, std::abs(y));
      }
      return peak;
   };

   q::basic_ladder<q::detail::moog_cell, 3> three{1_kHz, sps, 1.0f};
   q::basic_ladder<q::detail::moog_cell, 6> six{1_kHz, sps, 1.0f};

   CHECK(rings(three) > 0.01f);
   CHECK(rings(six) > 0.01f);
}
