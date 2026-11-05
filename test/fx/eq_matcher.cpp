/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/fx/eq_matcher.hpp>

#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fft/lpc_fit.hpp>
#include <q/fft/spectral_learner.hpp>
#include <q/fx/biquad.hpp>
#include <q/support/literals.hpp>

#include <cmath>
#include <complex>
#include <numbers>
#include <random>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;
using Catch::Matchers::WithinRel;

namespace
{
   constexpr std::size_t order = 32;
   constexpr float sps = 48000.0f;

   std::vector<float> some_k(float a, float b)
   {
      std::vector<float> k(order, 0.0f);
      k[0] = a;
      k[1] = b;
      return k;
   }

   // |A(f)|, A from its reflection coefficients (the step-up recursion).
   double magnitude(std::vector<float> const& k, double f)
   {
      std::vector<double> a(order + 1, 0.0), prev;
      a[0] = 1;
      for (std::size_t m = 1; m <= order; ++m)
      {
         prev = a;
         for (std::size_t j = 1; j < m; ++j)
            a[j] = prev[j] + k[m - 1] * prev[m - j];
         a[m] = k[m - 1];
      }
      std::complex<double> z = 0;
      auto const w = 2 * std::numbers::pi * f / sps;
      for (std::size_t j = 0; j != a.size(); ++j)
         z += a[j] * std::polar(1.0, -w * double(j));
      return std::abs(z);
   }

   double sine_gain(q::eq_matcher& m, double f)
   {
      auto const n = std::size_t(sps);
      double in = 0, out = 0;
      for (std::size_t i = 0; i != n; ++i)
      {
         auto const s = float(std::sin(2 * std::numbers::pi * f * i / sps));
         auto const y = m(s);
         if (i >= n / 2)
         {
            in += s * s;
            out += y * y;
         }
      }
      return std::sqrt(out / in);
   }

   // A synthetic instrument, as in spectral_learner's test.
   std::vector<float> play(q::frequency resonance, int notes)
   {
      q::lowpass body{resonance, sps, 3.0};
      std::vector<float> out;
      for (int n = 0; n != notes; ++n)
      {
         auto const f0 = 55.0 * std::exp2(36.0 * n / notes / 12.0);
         for (std::size_t i = 0; i != std::size_t(sps); ++i)
         {
            auto const t = double(i) / sps;
            double s = 0;
            for (int k = 1; k * f0 < 0.45 * sps; ++k)
               s += std::sin(2 * std::numbers::pi * k * f0 * t) / k;
            out.push_back(body(float(0.3 * s * std::exp(-3.0 * t))));
         }
      }
      return out;
   }

   // A learned model: its reflection coefficients and level.
   struct model
   {
      std::vector<float> k = std::vector<float>(order);
      float level;
   };

   std::vector<float> envelope_db(std::vector<float> const& x)
   {
      q::spectral_learner learner{sps};
      learner(x);
      std::vector<float> db(learner.bins());
      learner.spectral_envelope(db);
      return db;
   }

   // The rms difference in dB, 200 Hz to 8 kHz, mean offset removed.
   double distance_db(std::vector<float> const& x, std::vector<float> const& y)
   {
      auto const bin_hz = sps / (2.0 * double(x.size() - 1));
      double sum = 0, sum2 = 0;
      std::size_t n = 0;
      for (auto i = std::size_t(200 / bin_hz); i * bin_hz <= 8000; ++i, ++n)
      {
         auto const d = double(x[i] - y[i]);
         sum += d;
         sum2 += d * d;
      }
      auto const mean = sum / double(n);
      return std::sqrt(std::max(sum2 / double(n) - mean * mean, 0.0));
   }

   model learn(std::vector<float> const& x)
   {
      q::spectral_learner learner{sps};
      learner(x);
      std::vector<float> db(learner.bins());
      learner.spectral_envelope(db);
      q::lpc_fit fit{learner.bins(), order};
      model m;
      m.level = float(std::sqrt(fit(db, m.k)));
      return m;
   }
}

TEST_CASE("eq_matcher: passes the input through at an amount of 0", "[eq]")
{
   q::eq_matcher m{order};
   m.source(some_k(-0.5f, 0.3f), 1.0f);
   m.target(some_k(-0.9f, 0.5f), 2.0f);
   m.amount(0.0f);
   std::mt19937 rng{3};
   std::uniform_real_distribution<float> u{-1.0f, 1.0f};
   for (int i = 0; i != 10000; ++i)
   {
      auto const s = u(rng);
      REQUIRE(m(s) == s);
   }
}

TEST_CASE("eq_matcher: applies B / A", "[eq]")
{
   auto const ka = some_k(-0.5f, 0.3f);
   auto const kb = some_k(-0.9f, 0.5f);
   for (double f : {200.0, 1000.0, 3000.0, 8000.0})
   {
      q::eq_matcher m{order};
      m.source(ka, 1.0f);
      m.target(kb, 2.0f);
      auto const expected = magnitude(ka, f) / magnitude(kb, f) * 2.0;
      CHECK_THAT(sine_gain(m, f), WithinRel(expected, 0.02));
   }
}

TEST_CASE("eq_matcher: matches one instrument to another", "[eq]")
{
   // Learn both, play the first through the match, learn the result: its
   // spectral envelope takes the second one's shape.
   auto const a = play(1000_Hz, 20);
   auto const b = play(3500_Hz, 20);
   auto const ma = learn(a);
   auto const mb = learn(b);

   q::eq_matcher m{order};
   m.source(ma.k, ma.level);
   m.target(mb.k, mb.level);
   std::vector<float> y(a.size());
   for (std::size_t i = 0; i != a.size(); ++i)
      y[i] = m(a[i]);

   auto const da = envelope_db(a);
   auto const db = envelope_db(b);
   auto const dy = envelope_db(y);
   auto const before = distance_db(da, db);
   auto const after = distance_db(dy, db);
   INFO("from " << before << " dB to " << after << " dB");
   CHECK(after < 0.3 * before);
}
