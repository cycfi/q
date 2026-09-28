/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/ladder.hpp>
#include <q/support/literals.hpp>

#include <cmath>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr auto sps = 48000.0f;
   constexpr auto pi = 3.14159265358979323846;

   // The amplitude a filter settles at for a sine at f, in dB.
   template <typename F>
   float response_db(F& filt, float f, int cycles = 200)
   {
      auto step = 2.0 * pi * f / sps;
      auto n = int(cycles * sps / f);
      auto peak = 0.0f;
      for (int i = 0; i != n; ++i)
      {
         auto y = filt(float(std::sin(step * i)));
         if (i > n / 2)                         // after it settles
            peak = std::max(peak, std::abs(y));
      }
      return 20.0f * std::log10(std::max(peak, 1e-9f));
   }

   // The amplitude of the k-th harmonic of f in a buffer, by correlation.
   float harmonic(std::vector<float> const& y, float f, int k)
   {
      auto step = 2.0 * pi * f * k / sps;
      double re = 0, im = 0;
      for (std::size_t i = 0; i != y.size(); ++i)
      {
         re += y[i] * std::cos(step * i);
         im += y[i] * std::sin(step * i);
      }
      return float(2.0 * std::sqrt(re * re + im * im) / y.size());
   }

   // A sine through the filter, past the settling time.
   template <typename F>
   std::vector<float> run(F& filt, float f, float amp, int cycles = 64)
   {
      auto step = 2.0 * pi * f / sps;
      auto n = int(cycles * sps / f);
      std::vector<float> out;
      out.reserve(n);
      for (int i = 0; i != 4 * n; ++i)         // settle, then collect
      {
         auto y = filt(amp * float(std::sin(step * i)));
         if (i >= 3 * n)
            out.push_back(y);
      }
      return out;
   }
}

TEST_CASE("ota_ladder: four one-poles, as the analog prototype has")
{
   // Four cascaded one-poles: |H| = (1 + (f/fc)^2)^-2, which is -12.04 dB at
   // the corner and -27.96 at an octave above it. The asymptote is -24 dB an
   // octave, but only well past the corner: between 2fc and 4fc the prototype
   // itself gives 21.2 dB, so that is what to expect there.
   q::ota_ladder f{1_kHz, sps};

   auto prototype = [](float ratio)
   {
      auto m = 1.0f / ((1.0f + ratio * ratio) * (1.0f + ratio * ratio));
      return 20.0f * std::log10(m);
   };

   auto at_fc = response_db(f, 1000.0f);
   f = 0.0f;
   auto at_2fc = response_db(f, 2000.0f);
   f = 0.0f;
   auto at_4fc = response_db(f, 4000.0f);

   CHECK(at_fc == Approx(prototype(1.0f)).margin(0.6f));
   CHECK(at_2fc == Approx(prototype(2.0f)).margin(0.8f));
   CHECK(at_4fc == Approx(prototype(4.0f)).margin(1.5f));
}

TEST_CASE("ota_ladder: the linear mode is moog_ladder's linear mode")
{
   // Same topology, same zero-delay resolution: the difference is the cell,
   // and at drive 0 there is no cell.
   q::ota_ladder a{800_Hz, sps, 0.7f};
   q::moog_ladder b{800_Hz, sps, 0.7f};

   for (int i = 0; i != 2000; ++i)
   {
      auto x = float(std::sin(0.01 * i)) + 0.3f * float(std::sin(0.13 * i));
      CHECK(a(x) == Approx(b(x)).margin(1e-6f));
   }
}

TEST_CASE("ota_ladder: it self-oscillates at r = 1")
{
   q::ota_ladder f{1_kHz, sps, 1.0f};

   f(1.0f);                                    // one impulse, then silence
   auto peak = 0.0f;
   for (int i = 0; i != 20000; ++i)
   {
      auto y = f(0.0f);
      if (i > 10000)
         peak = std::max(peak, std::abs(y));
   }
   CHECK(peak > 0.01f);                        // still ringing
}

TEST_CASE("ota_ladder: the cell distorts in the second harmonic")
{
   // The CEM3320 data sheet: distortion in the passband is "predominantly
   // second harmonic". The bias is what produces it.
   q::ota_ladder f{4_kHz, sps, 0.0f};
   f.drive(2.0f);                              // the default asymmetry

   auto y = run(f, 500.0f, 0.8f);
   auto h2 = harmonic(y, 500.0f, 2);
   auto h3 = harmonic(y, 500.0f, 3);

   CHECK(h2 > 4.0f * h3);                      // predominantly second
}

TEST_CASE("ota_ladder: a symmetric cell has no second harmonic")
{
   // asymmetry 0 is a ladder's symmetric curve: odd harmonics only.
   q::ota_ladder f{4_kHz, sps, 0.0f};
   f.drive(2.0f);
   f.asymmetry(0.0f);

   auto y = run(f, 500.0f, 0.8f);
   auto h2 = harmonic(y, 500.0f, 2);
   auto h3 = harmonic(y, 500.0f, 3);

   CHECK(h3 > h2);
}

TEST_CASE("ota_ladder: driving it hard stays bounded")
{
   q::ota_ladder f{2_kHz, sps, 0.95f};
   f.drive(20.0f);

   for (int i = 0; i != 20000; ++i)
   {
      auto y = f(8.0f * float(std::sin(0.05 * i)));
      REQUIRE(std::isfinite(y));
      REQUIRE(std::abs(y) < 20.0f);
   }
}
