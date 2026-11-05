/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/utility/levinson.hpp>

#include <cmath>
#include <random>
#include <vector>

namespace q = cycfi::q;
using Catch::Matchers::WithinAbs;

namespace
{
   // The direct-form coefficients a[1] .. a[N] from the reflection
   // coefficients (the step-up recursion).
   template <std::size_t N>
   std::array<double, N + 1> step_up(std::array<double, N> const& k)
   {
      std::array<double, N + 1> a{}, prev{};
      a[0] = 1.0;
      for (std::size_t m = 1; m <= N; ++m)
      {
         prev = a;
         for (std::size_t j = 1; j < m; ++j)
            a[j] = prev[j] + k[m - 1] * prev[m - j];
         a[m] = k[m - 1];
      }
      return a;
   }
}

TEST_CASE("levinson: a first-order process", "[levinson]")
{
   // r[l] = rho^l, the autocorrelation of x[n] = rho x[n-1] + e[n]
   double const rho = 0.8;
   std::array<double, 5> r;
   for (std::size_t l = 0; l != r.size(); ++l)
      r[l] = std::pow(rho, double(l));

   std::array<double, 4> k;
   std::array<double, 5> a;
   auto const err = q::levinson(r, k, a);

   CHECK_THAT(k[0], WithinAbs(-rho, 1e-12));
   for (std::size_t m = 1; m != k.size(); ++m)
      CHECK_THAT(k[m], WithinAbs(0.0, 1e-12));
   CHECK_THAT(err, WithinAbs(1.0 - rho * rho, 1e-12));
}

TEST_CASE("levinson: a second-order process from its samples", "[levinson]")
{
   // x[n] = 1.3 x[n-1] - 0.6 x[n-2] + e[n]: A(z) = 1 - 1.3 z^-1 + 0.6 z^-2
   std::mt19937 rng{7};
   std::normal_distribution<double> noise;
   std::size_t const n = 200000;
   std::vector<double> x(n, 0.0);
   for (std::size_t i = 2; i != n; ++i)
      x[i] = 1.3 * x[i - 1] - 0.6 * x[i - 2] + noise(rng);

   std::array<double, 5> r{};
   for (std::size_t l = 0; l != r.size(); ++l)
   {
      for (std::size_t i = l; i != n; ++i)
         r[l] += x[i] * x[i - l];
   }

   std::array<double, 4> k;
   std::array<double, 5> a;
   q::levinson(r, k, a);

   CHECK_THAT(a[1], WithinAbs(-1.3, 0.02));
   CHECK_THAT(a[2], WithinAbs(0.6, 0.02));
   CHECK_THAT(a[3], WithinAbs(0.0, 0.02));
   CHECK_THAT(a[4], WithinAbs(0.0, 0.02));

   // a is the polynomial of the k.
   auto const up = step_up<4>(k);
   for (std::size_t j = 0; j != a.size(); ++j)
      CHECK_THAT(a[j], WithinAbs(up[j], 1e-12));
}

TEST_CASE("levinson: silence", "[levinson]")
{
   std::array<double, 3> r{};
   std::array<double, 2> k{0.5, 0.5};
   std::array<double, 3> a{0.5, 0.5, 0.5};
   CHECK(q::levinson(r, k, a) == 0.0);
   CHECK(k[0] == 0.0);
   CHECK(k[1] == 0.0);
   CHECK(a[0] == 1.0);
   CHECK(a[1] == 0.0);
   CHECK(a[2] == 0.0);
}
