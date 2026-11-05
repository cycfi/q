/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/fft/lpc_fit.hpp>

#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <cmath>
#include <complex>
#include <numbers>
#include <vector>

namespace q = cycfi::q;
using Catch::Matchers::WithinAbs;

namespace
{
   constexpr std::size_t n = 4096;
   constexpr std::size_t bins = n / 2 + 1;

   // |A(f)|^2 at bin b, A(z) = a[0] + a[1] z^-1 + ...
   double mag2(std::vector<double> const& a, std::size_t b)
   {
      std::complex<double> z = 0;
      auto const w = 2 * std::numbers::pi * double(b) / double(n);
      for (std::size_t j = 0; j != a.size(); ++j)
         z += a[j] * std::polar(1.0, -w * double(j));
      return std::norm(z);
   }

   // The spectral envelope of 1 / A(z), in dB on the bins.
   std::vector<float> all_pole_db(std::vector<double> const& a)
   {
      std::vector<float> db(bins);
      for (std::size_t b = 0; b != bins; ++b)
         db[b] = float(-10 * std::log10(mag2(a, b)));
      return db;
   }
}

TEST_CASE("lpc_fit: recovers an all-pole model", "[lpc]")
{
   // A(z) = 1 - 1.3 z^-1 + 0.6 z^-2: a resonance, as 2 reflection
   // coefficients k2 = 0.6, k1 = -1.3 / (1 + 0.6).
   std::vector<double> const a{1.0, -1.3, 0.6};
   auto const db = all_pole_db(a);

   q::lpc_fit fit{bins, 8};
   std::vector<float> k(8);
   auto const err = fit(db, k);

   CHECK(err > 0);
   CHECK_THAT(k[0], WithinAbs(-1.3 / 1.6, 1e-3));
   CHECK_THAT(k[1], WithinAbs(0.6, 1e-3));
   for (std::size_t m = 2; m != k.size(); ++m)
      CHECK_THAT(k[m], WithinAbs(0.0, 1e-3));

   auto const p = fit.polynomial();
   REQUIRE(p.size() == 9);
   CHECK_THAT(p[0], WithinAbs(1.0, 1e-12));
   CHECK_THAT(p[1], WithinAbs(-1.3, 1e-3));
   CHECK_THAT(p[2], WithinAbs(0.6, 1e-3));

   // response() is |A(f)|^2 of the fit, on the bins.
   std::vector<float> r(bins);
   fit.response(r);
   std::vector<double> const pa(p.begin(), p.end());
   for (std::size_t b = 0; b < bins; b += 64)
      CHECK_THAT(r[b], WithinAbs(mag2(pa, b), 1e-3 * mag2(pa, b) + 1e-6));
}

TEST_CASE("lpc_fit: a flat spectral envelope", "[lpc]")
{
   std::vector<float> const db(bins, -12.0f);
   q::lpc_fit fit{bins, 16};
   std::vector<float> k(16);
   fit(db, k);
   for (auto km : k)
      CHECK_THAT(km, WithinAbs(0.0, 1e-4));
}
