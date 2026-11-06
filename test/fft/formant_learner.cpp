/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fft/formant_learner.hpp>
#include <q/fx/biquad.hpp>
#include <q/support/literals.hpp>

#include <cmath>
#include <complex>
#include <memory>
#include <numbers>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   using table_type = q::formant_table;
   constexpr float sps = 48000.0f;

   // A synthetic instrument: notes with 1/k harmonics, decaying over
   // their second, from A1 up three octaves, through a fixed resonance.
   std::vector<float> play(q::frequency resonance, int notes)
   {
      q::lowpass body{resonance, sps, 3.0};
      std::vector<float> out;
      for (int n = 0; n != notes; ++n)
      {
         auto const f0 = 55.0 * std::exp2(36.0 * n / notes / 12.0);
         auto const len = std::size_t(sps);
         for (std::size_t i = 0; i != len; ++i)
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

   // The spectral envelope a row's model gives at f, in dB (1/|A|, its
   // level).
   double model_db(table_type const& t, std::size_t row, double f)
   {
      auto const k = t.k(row);
      auto const order = k.size();
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
      return 20 * std::log10(t.level(row) / std::abs(z));
   }

   // The frequency of a row's highest point between lo and hi.
   double peak_hz(table_type const& t, std::size_t row, double lo, double hi)
   {
      double best = lo, top = -1e9;
      for (double f = lo; f <= hi; f *= 1.01)
      {
         auto const v = model_db(t, row, f);
         if (v > top)
         {
            top = v;
            best = f;
         }
      }
      return best;
   }

   std::size_t row_at(int semitones)
   {
      return std::size_t((semitones - table_type::lo) * table_type::steps);
   }
}

TEST_CASE("formant_learner: learns a fixed resonance", "[formant]")
{
   q::formant_learner learner{sps};
   auto const x = play(3000_Hz, 30);
   learner(x);

   CHECK(learner.frames() > 0);
   CHECK(learner.coverage() > 0.8f);

   auto table = std::make_unique<table_type>();
   REQUIRE(learner.fit(*table));

   // F peaks at the resonance; F(f/r) at r times it.
   auto const f1 = peak_hz(*table, table->unshifted(), 1000, 10000);
   CHECK(std::abs(f1 / 3000 - 1) < 0.15);
   auto const f2 = peak_hz(*table, row_at(-12), 500, 10000);
   CHECK(std::abs(f2 / 1500 - 1) < 0.15);
}

TEST_CASE("formant_learner: a well-formed table", "[formant]")
{
   q::formant_learner learner{sps};
   learner(play(3000_Hz, 30));
   auto table = std::make_unique<table_type>();
   REQUIRE(learner.fit(*table));

   for (std::size_t i = 0; i != table_type::num_rows; ++i)
   {
      REQUIRE(std::isfinite(table->level(i)));
      REQUIRE(table->level(i) > 0.0f);
      for (auto k : table->k(i))
      {
         REQUIRE(std::isfinite(k));
         REQUIRE(std::abs(k) < 1.0f);
      }
   }

   // Unshifted, the correction is F / F: the same model, the same level.
   std::vector<float> k(table->order());
   float level;
   table->at(1.0f, k, level);
   auto const base = table->k(table->unshifted());
   CHECK(std::equal(k.begin(), k.end(), base.begin()));
   CHECK(level == table->level(table->unshifted()));
}

TEST_CASE("formant_learner: at order 24", "[formant]")
{
   q::formant_learner learner{sps, 24};
   learner(play(3000_Hz, 30));
   auto table = std::make_unique<table_type>(24);
   REQUIRE(learner.fit(*table));
   for (std::size_t i = 0; i != table_type::num_rows; ++i)
   {
      REQUIRE(table->level(i) > 0.0f);
      for (auto k : table->k(i))
         REQUIRE(std::abs(k) < 1.0f);
   }

   // The same resonance as at order 32, and a corrector takes the table.
   auto const f1 = peak_hz(*table, table->unshifted(), 1000, 10000);
   CHECK(std::abs(f1 / 3000 - 1) < 0.15);

   q::formant_corrector fc{24};
   fc.table(table.get());
   CHECK(fc(0.5f, 2.0f) != 0.0f);
}

TEST_CASE("formant_learner: a restored learner fits the same table", "[formant]")
{
   q::formant_learner a{sps};
   a(play(3000_Hz, 30));
   q::spectral_learner::state s;
   a.save(s);

   q::formant_learner b{sps};
   REQUIRE(b.restore(s));

   auto ta = std::make_unique<table_type>();
   auto tb = std::make_unique<table_type>();
   REQUIRE(a.fit(*ta));
   REQUIRE(b.fit(*tb));
   for (std::size_t i = 0; i != table_type::num_rows; ++i)
   {
      CHECK(ta->level(i) == tb->level(i));
      CHECK(std::equal(ta->k(i).begin(), ta->k(i).end(), tb->k(i).begin()));
   }
}
