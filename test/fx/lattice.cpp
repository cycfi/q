/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fx/lattice.hpp>

#include <cmath>
#include <random>
#include <vector>

namespace q = cycfi::q;

namespace
{
   constexpr std::size_t order = 8;
   using coefficients = std::array<float, order>;

   coefficients some_k(float phase)
   {
      coefficients k;
      for (std::size_t m = 0; m != order; ++m)
         k[m] = 0.9f * std::sin(phase + 0.7f * float(m));
      return k;
   }

   // The direct-form coefficients of A(z) from k (the step-up recursion).
   std::array<double, order + 1> step_up(coefficients const& k)
   {
      std::array<double, order + 1> a{}, prev{};
      a[0] = 1.0;
      for (std::size_t m = 1; m <= order; ++m)
      {
         prev = a;
         for (std::size_t j = 1; j < m; ++j)
            a[j] = prev[j] + k[m - 1] * prev[m - j];
         a[m] = k[m - 1];
      }
      return a;
   }

   std::vector<float> noise(std::size_t n)
   {
      std::mt19937 rng{11};
      std::uniform_real_distribution<float> u{-1.0f, 1.0f};
      std::vector<float> x(n);
      for (auto& s : x)
         s = u(rng);
      return x;
   }
}

TEST_CASE("lattice_fir: is the direct-form A(z)", "[lattice]")
{
   auto const k = some_k(0.3f);
   auto const a = step_up(k);
   auto const x = noise(2000);

   q::lattice_fir fir{order};
   for (std::size_t i = 0; i != x.size(); ++i)
   {
      double direct = 0;
      for (std::size_t j = 0; j <= order && j <= i; ++j)
         direct += a[j] * x[i - j];
      REQUIRE(std::abs(fir(x[i], k) - direct) < 1e-5);
   }
}

TEST_CASE("lattice_iir: undoes lattice_fir", "[lattice]")
{
   auto const x = noise(20000);
   q::lattice_fir fir{order};
   q::lattice_iir iir{order};

   SECTION("fixed coefficients")
   {
      auto const k = some_k(1.1f);
      for (auto s : x)
         REQUIRE(std::abs(iir(fir(s, k), k) - s) < 1e-4);
   }

   SECTION("coefficients that change every sample")
   {
      for (std::size_t i = 0; i != x.size(); ++i)
      {
         auto const k = some_k(0.001f * float(i));
         REQUIRE(std::abs(iir(fir(x[i], k), k) - x[i]) < 1e-4);
      }
   }
}
