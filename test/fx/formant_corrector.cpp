/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fx/formant_corrector.hpp>

#include <array>
#include <cmath>
#include <complex>
#include <memory>
#include <numbers>
#include <random>
#include <vector>

namespace q = cycfi::q;
using Catch::Matchers::WithinRel;

namespace
{
   using table_type = q::formant_table;
   constexpr std::size_t order = 32;
   using coefficients = std::array<float, order>;
   constexpr float sps = 48000.0f;

   struct row
   {
      coefficients   k;
      float          level;
   };

   coefficients some_k(float a, float b)
   {
      coefficients k{};
      k[0] = a;
      k[1] = b;
      return k;
   }

   // |A(f)|, A from its reflection coefficients (the step-up recursion).
   double magnitude(coefficients const& k, double f)
   {
      std::array<double, order + 1> a{}, prev{};
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

   // A table whose unshifted row is `base` and every other row `other`.
   std::unique_ptr<table_type> two_rows(row base, row other)
   {
      auto t = std::make_unique<table_type>(order);
      for (std::size_t i = 0; i != table_type::num_rows; ++i)
      {
         auto const& r = i == t->unshifted() ? base : other;
         std::copy(r.k.begin(), r.k.end(), t->k(i).begin());
         t->level(i) = r.level;
      }
      return t;
   }

   q::formant_corrector corrector(table_type const* table)
   {
      q::formant_corrector fc{order};
      fc.table(table);
      return fc;
   }

   double sine_gain(q::formant_corrector& fc, double f, float ratio)
   {
      auto const n = std::size_t(sps);
      double in = 0, out = 0;
      for (std::size_t i = 0; i != n; ++i)
      {
         auto const s = float(std::sin(2 * std::numbers::pi * f * i / sps));
         auto const y = fc(s, ratio);
         if (i >= n / 2)
         {
            in += s * s;
            out += y * y;
         }
      }
      return std::sqrt(out / in);
   }
}

TEST_CASE("formant_corrector: passes the input through", "[formant]")
{
   auto const table = two_rows(
      {some_k(-0.9f, 0.5f), 2.0f}, {some_k(-0.5f, 0.3f), 1.0f});
   std::mt19937 rng{3};
   std::uniform_real_distribution<float> u{-1.0f, 1.0f};

   SECTION("with no table")
   {
      q::formant_corrector fc;
      for (int i = 0; i != 10000; ++i)
      {
         auto const s = u(rng);
         REQUIRE(fc(s, 2.0f) == s);
      }
   }

   SECTION("unshifted")
   {
      auto fc = corrector(table.get());
      for (int i = 0; i != 10000; ++i)
      {
         auto const s = u(rng);
         REQUIRE(fc(s, 1.0f) == s);
      }
   }

   SECTION("at an amount of 0")
   {
      auto fc = corrector(table.get());
      fc.amount(0.0f);
      for (int i = 0; i != 10000; ++i)
      {
         auto const s = u(rng);
         REQUIRE(fc(s, 2.0f) == s);
      }
   }
}

TEST_CASE("formant_corrector: starts again from rest", "[formant]")
{
   auto const table = two_rows(
      {some_k(-0.9f, 0.5f), 2.0f}, {some_k(-0.5f, 0.3f), 1.0f});
   std::mt19937 rng{5};
   std::uniform_real_distribution<float> u{-1.0f, 1.0f};

   // Shifted, then at rest, then shifted again: from there on it matches
   // a fresh corrector given the same samples.
   auto fc = corrector(table.get());
   for (int i = 0; i != 1024; ++i)
      fc(u(rng), 2.0f);
   for (int i = 0; i != 1024; ++i)
      REQUIRE(fc(u(rng), 1.0f) != 0.0f);

   auto fresh = corrector(table.get());
   for (int i = 0; i != 1024; ++i)
   {
      auto const s = u(rng);
      REQUIRE(fc(s, 2.0f) == fresh(s, 2.0f));
   }
}

TEST_CASE("formant_corrector: applies F / F(f/r)", "[formant]")
{
   row const base{some_k(-0.9f, 0.5f), 2.0f};
   row const other{some_k(-0.5f, 0.3f), 1.0f};
   auto const table = two_rows(base, other);

   for (double f : {200.0, 1000.0, 3000.0, 8000.0})
   {
      auto fc = corrector(table.get());
      auto const expected = magnitude(other.k, f) / magnitude(base.k, f)
         * double(base.level / other.level);
      CHECK_THAT(sine_gain(fc, f, 2.0f), WithinRel(expected, 0.02));
   }
}
