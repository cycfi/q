/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/support/literals.hpp>
#include <q/synth/waveguide.hpp>
#include <q/synth/gen/noise_gen.hpp>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr float sps = 48000;

   // A plain pluck: one period of white noise.
   std::vector<float> ring(
      q::waveguide& s, float hz, std::size_t n)
   {
      q::white_noise_gen noise;
      auto const burst = std::size_t(sps / hz);
      std::vector<float> out(n);
      for (std::size_t i = 0; i != n; ++i)
         out[i] = s(i < burst ? noise() : 0.0f);
      return out;
   }

   // The period, from the normalized autocorrelation's highest peak
   // between lo and hi samples, refined by a parabola through it.
   double period(std::vector<float> const& x, std::size_t a,
      std::size_t n, double lo, double hi)
   {
      auto r = [&](std::size_t lag)
      {
         double s = 0, e0 = 0, e1 = 0;
         for (std::size_t i = a; i != a + n; ++i)
         {
            s += x[i] * x[i + lag];
            e0 += x[i] * x[i];
            e1 += x[i + lag] * x[i + lag];
         }
         return s / std::sqrt(e0 * e1 + 1e-30);
      };
      auto best = std::size_t(lo);
      for (auto lag = std::size_t(lo); lag <= std::size_t(hi); ++lag)
      {
         if (r(lag) > r(best))
            best = lag;
      }
      auto const y0 = r(best - 1), y1 = r(best), y2 = r(best + 1);
      return best + 0.5 * (y0 - y2) / (y0 - 2 * y1 + y2);
   }

   double cents(double measured, double expected)
   {
      return 1200 * std::log2(measured / expected);
   }

   double rms_db(std::vector<float> const& x, std::size_t a,
      std::size_t n)
   {
      double e = 0;
      for (std::size_t i = a; i != a + n; ++i)
         e += x[i] * x[i];
      return 10 * std::log10(e / n + 1e-30);
   }
}

TEST_CASE("An excitation is heard on the sample it arrives")
{
   q::waveguide s{110_Hz, sps};
   CHECK(s(1.0f) == 1.0f);
   CHECK(s(0.0f) == 0.0f);        // the loop has not come round yet
}

TEST_CASE("Silence stays silent, and reset clears the string")
{
   q::waveguide s{110_Hz, sps};
   for (int i = 0; i != 1000; ++i)
      REQUIRE(s(0.0f) == 0.0f);

   ring(s, 110, 2000);
   s.reset();
   for (int i = 0; i != 1000; ++i)
      REQUIRE(s(0.0f) == 0.0f);
}

TEST_CASE("It rings at its frequency")
{
   for (double hz : {82.41, 110.0, 196.0, 329.63, 659.26, 1318.5})
   {
      q::waveguide s{80_Hz, sps};
      s.frequency(q::frequency{hz});
      auto const x = ring(s, float(hz), std::size_t(sps));
      auto const p0 = sps / hz;
      auto const p = period(x, std::size_t(0.2 * sps), 4096,
         0.8 * p0, 1.2 * p0);
      auto const c = cents(sps / p, hz);
      INFO(hz << " Hz: " << c << " cents");
      CHECK(std::abs(c) < 0.5);
   }
}

TEST_CASE("It retunes while it rings")
{
   q::waveguide s{80_Hz, sps};
   s.frequency(110_Hz);
   auto x = ring(s, 110, std::size_t(0.3 * sps));
   s.frequency(220_Hz);
   for (std::size_t i = 0; i != std::size_t(0.5 * sps); ++i)
      x.push_back(s(0.0f));
   auto const p = period(x, std::size_t(0.5 * sps), 4096, 180, 260);
   auto const c = cents(sps / p, 220.0);
   INFO("after the retune: " << c << " cents");
   CHECK(std::abs(c) < 0.5);
}

TEST_CASE("It decays by its T60")
{
   // At 110 Hz the average's own loss is small at the fundamental; the
   // level a second later is 60 dB down, give or take the higher
   // harmonics dying faster.
   q::waveguide s{80_Hz, sps, 1_s};
   s.frequency(110_Hz);
   auto const x = ring(s, 110, std::size_t(1.5 * sps));
   auto const a = rms_db(x, std::size_t(0.2 * sps), 4800);
   auto const b = rms_db(x, std::size_t(1.2 * sps), 4800);
   INFO("drop over 1 s: " << a - b << " dB");
   CHECK(a - b > 58.0);
   CHECK(a - b < 70.0);

   // A short T60 damps it.
   s.decay(50_ms);
   for (int i = 0; i != int(0.05 * sps); ++i)
      s(0.0f);
   std::vector<float> tail;
   for (int i = 0; i != 4800; ++i)
      tail.push_back(s(0.0f));
   CHECK(rms_db(tail, 0, 4800) < b - 40);
}

namespace
{
   // The first output sample at or past `from` whose magnitude passes
   // `level`, and its sign.
   std::pair<std::size_t, float> arrival(
      std::vector<float> const& x, std::size_t from, float level)
   {
      for (auto i = from; i != x.size(); ++i)
         if (std::abs(x[i]) > level)
            return {i, x[i]};
      return {x.size(), 0.0f};
   }

   std::vector<float> impulse(q::waveguide& s, std::size_t n)
   {
      std::vector<float> out(n);
      for (std::size_t i = 0; i != n; ++i)
         out[i] = s(i == 0? 1.0f : 0.0f);
      return out;
   }
}

TEST_CASE("A pluck point reflects off the bridge inverted")
{
   // 110 Hz open: the open string is 218.2 samples one way, so a pluck at
   // 0.2 is 44 samples from the bridge. The half going to the bridge comes
   // back inverted after 44 samples; the half going to the fret returns
   // upright after it reflects at both ends, about a period less 44.
   q::waveguide s{110_Hz, sps};
   s.pluck(0.2f);
   auto const x = impulse(s, 1000);
   auto const first = arrival(x, 0, 0.05f);
   CHECK(first.first == 44);
   CHECK(first.second < 0);
   auto const second = arrival(x, first.first + 3, 0.05f);
   auto const p0 = sps / 110.0f;
   CHECK(std::abs(float(second.first) - (p0 - 44)) <= 2.0f);
   CHECK(second.second > 0);
}

TEST_CASE("A fret moves neither the pluck point nor the pickup")
{
   // Up an octave the string is half as long, but the pluck point is
   // still 44 samples from the bridge.
   q::waveguide s{110_Hz, sps};
   s.pluck(0.2f);
   s.frequency(220_Hz);
   auto const x = impulse(s, 1000);
   CHECK(arrival(x, 0, 0.05f).first == 44);
   auto const second = arrival(x, 47, 0.05f);
   CHECK(std::abs(float(second.first) - (sps / 220.0f - 44)) <= 2.0f);
}

TEST_CASE("It rings at its frequency wherever it is plucked and read")
{
   using rd = q::waveguide::readout;
   for (auto r : {rd::near_end, rd::rail, rd::displacement})
   {
      for (double hz : {110.0, 220.0, 330.0})
      {
         q::waveguide s{110_Hz, sps};
         s.pluck(0.145f);
         s.pickup(0.0386f, r);
         s.frequency(q::frequency{hz});
         auto const x = ring(s, float(hz), std::size_t(sps));
         auto const p0 = sps / hz;
         auto const p = period(x, std::size_t(0.2 * sps), 4096,
            0.8 * p0, 1.2 * p0);
         auto const c = cents(sps / p, hz);
         INFO("readout " << int(r) << ", " << hz << " Hz: " << c << " cents");
         CHECK(std::abs(c) < 0.5);
      }
   }
}

TEST_CASE("A place the string is too short for is held where it fits")
{
   q::waveguide s{110_Hz, sps};
   s.pluck(0.9f);
   s.frequency(880_Hz);
   auto const x = ring(s, 880, 4800);
   for (auto v : x)
      REQUIRE(std::isfinite(v));
   CHECK(rms_db(x, 2400, 2400) > -80);
}

namespace
{
   // Energy in a band, from an FFT-free sum of squares after a crude
   // band-pass: the difference of two one-pole lowpasses.
   double band_db(std::vector<float> const& x, std::size_t a, std::size_t n,
      double lo, double hi)
   {
      auto pole = [](double f) { return std::exp(-2 * M_PI * f / sps); };
      double const pl = pole(lo), ph = pole(hi);
      double yl = 0, yh = 0, e = 0;
      for (std::size_t i = 0; i != a + n; ++i)
      {
         yl = (1 - pl) * x[i] + pl * yl;
         yh = (1 - ph) * x[i] + ph * yh;
         if (i >= a)
            e += (yh - yl) * (yh - yl);
      }
      return 10 * std::log10(e / n + 1e-30);
   }
}

TEST_CASE("Tuning does not dull the highs")
{
   // No lowpass (loss 0) and a very long decay: the band from 4 to 8 kHz
   // should keep its level, the allpass passing it whole.
   q::waveguide s{100_Hz, sps, q::duration{1000.0}};
   s.loss(0.0f);
   s.frequency(q::frequency{123.4});
   std::vector<float> x(static_cast<std::size_t>(sps));
   for (std::size_t i = 0; i != x.size(); ++i)
      x[i] = s(i < 8? 1.0f - i / 8.0f : 0.0f);
   auto const a = band_db(x, std::size_t(0.05 * sps), 4800, 4000, 8000);
   auto const b = band_db(x, std::size_t(0.8 * sps), 4800, 4000, 8000);
   INFO("4-8 kHz drop over 0.75 s: " << a - b << " dB");
   CHECK(std::abs(a - b) < 1.0);
}

TEST_CASE("It rings at its frequency with a one-pole loss")
{
   for (double hz : {82.41, 196.0, 659.26})
   {
      q::waveguide s{80_Hz, sps};
      s.loss(0.1f);
      s.frequency(q::frequency{hz});
      auto const x = ring(s, float(hz), std::size_t(sps));
      auto const p0 = sps / hz;
      auto const p = period(x, std::size_t(0.2 * sps), 4096,
         0.8 * p0, 1.2 * p0);
      auto const c = cents(sps / p, hz);
      INFO(hz << " Hz: " << c << " cents");
      CHECK(std::abs(c) < 0.5);
   }
}

TEST_CASE("Stiffness sets the partials sharp")
{
   // A stiff string's partial k sits at k f0 sqrt((1 + B k^2) / (1 + B)).
   // Measured by FFT-free peak search: each partial's frequency from the
   // phase advance of a complex demodulation near its expected place.
   float const b = 1e-4f;
   double const f0 = 110.0;
   q::waveguide s{100_Hz, sps, q::duration{20.0}};
   s.stiffness(b);
   s.frequency(q::frequency{f0});
   std::vector<float> x(static_cast<std::size_t>(1.5 * sps));
   for (std::size_t i = 0; i != x.size(); ++i)
      x[i] = s(i < 8? 1.0f - i / 8.0f : 0.0f);

   auto measure = [&](double guess)
   {
      // the frequency whose demodulated phase stays still, searched
      // around the guess (a 1 s window resolves well under a cent here)
      auto const a = std::size_t(0.3 * sps), n = std::size_t(1.0 * sps);
      auto power = [&](double f)
      {
         double re = 0, im = 0;
         for (std::size_t i = a; i != a + n; ++i)
         {
            auto const ph = 2 * M_PI * f * i / sps;
            re += x[i] * std::cos(ph);
            im -= x[i] * std::sin(ph);
         }
         return re * re + im * im;
      };
      double best = guess, top = 0;
      for (double f = guess * 0.97; f <= guess * 1.03; f += guess * 0.0002)
      {
         auto const p = power(f);
         if (p > top) { top = p; best = f; }
      }
      return best;
   };
   for (int k : {1, 5, 10, 15})
   {
      auto const want = k * f0 * std::sqrt((1 + b * k * k) / (1 + b));
      auto const got = measure(want);
      auto const c = 1200 * std::log2(got / want);
      INFO("partial " << k << ": " << c << " cents off a stiff string's");
      CHECK(std::abs(c) < (k == 1? 1.0 : 8.0));
   }
   auto const h15 = measure(15 * f0 * std::sqrt((1 + b * 225) / (1 + b)));
   INFO("partial 15 over 15 f0: " << 1200 * std::log2(h15 / (15 * f0)) << " cents");
   CHECK(1200 * std::log2(h15 / (15 * f0)) > 10);
}
