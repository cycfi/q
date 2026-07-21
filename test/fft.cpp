/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fft/fft.hpp>
#include <q/support/literals.hpp>

#include <array>
#include <complex>
#include <cmath>

namespace q = cycfi::q;
using namespace q::literals;

// `fft`/`ifft`/`magspec` operate in place on an interleaved complex array:
//    data = [ re(0), im(0), re(1), im(1), ... , re(N-1), im(N-1) ]
// so an N-point transform needs 2N scalars, and bin k lives at
// (data[2k], data[2k+1]).

namespace
{
   constexpr double eps = 1e-9;

   // Naive O(N^2) DFT, used as an independent reference.
   template <std::size_t N>
   std::array<std::complex<double>, N> naive_dft(std::array<double, 2*N> const& d)
   {
      std::array<std::complex<double>, N> out{};
      for (std::size_t k = 0; k != N; ++k)
      {
         std::complex<double> acc{};
         for (std::size_t n = 0; n != N; ++n)
         {
            std::complex<double> x(d[2*n], d[2*n+1]);
            double a = -2.0 * q::pi * double(k) * double(n) / double(N);
            acc += x * std::complex<double>(std::cos(a), std::sin(a));
         }
         out[k] = acc;
      }
      return out;
   }

   // A deterministic complex signal for a given size.
   template <std::size_t N>
   std::array<double, 2*N> make_signal()
   {
      std::array<double, 2*N> d{};
      for (std::size_t n = 0; n != N; ++n)
      {
         d[2*n]   = std::sin(0.30 * n) + 0.5 * std::cos(0.11 * n * n);
         d[2*n+1] = 0.25 * std::sin(0.70 * n) - 0.1 * n / double(N);
      }
      return d;
   }
}

TEMPLATE_TEST_CASE_SIG("fft matches the naive DFT", "[fft]",
   ((std::size_t N), N), 2, 4, 8, 16, 32, 64)
{
   auto d = make_signal<N>();
   auto ref = naive_dft<N>(d);
   q::fft<N>(d.data());
   for (std::size_t k = 0; k != N; ++k)
   {
      REQUIRE_THAT(d[2*k],   Catch::Matchers::WithinAbs(ref[k].real(), 1e-6));
      REQUIRE_THAT(d[2*k+1], Catch::Matchers::WithinAbs(ref[k].imag(), 1e-6));
   }
}

TEST_CASE("fft: a complex exponential maps to a single bin", "[fft]")
{
   constexpr std::size_t N = 32;
   constexpr std::size_t k0 = 5;
   std::array<double, 2*N> d{};
   for (std::size_t n = 0; n != N; ++n)
   {
      double a = 2.0 * q::pi * k0 * n / double(N);
      d[2*n]   = std::cos(a);
      d[2*n+1] = std::sin(a);
   }
   q::fft<N>(d.data());
   for (std::size_t k = 0; k != N; ++k)
   {
      double mag = std::hypot(d[2*k], d[2*k+1]);
      REQUIRE_THAT(mag, Catch::Matchers::WithinAbs(k == k0 ? double(N) : 0.0, 1e-9));
   }
}

TEST_CASE("fft: a real cosine maps to conjugate-symmetric bins", "[fft]")
{
   constexpr std::size_t N = 32;
   constexpr std::size_t k0 = 6;
   std::array<double, 2*N> d{};
   for (std::size_t n = 0; n != N; ++n)
      d[2*n] = std::cos(2.0 * q::pi * k0 * n / double(N));   // imag stays 0
   q::fft<N>(d.data());
   // Energy N/2 at k0 and its mirror N-k0, zero elsewhere.
   for (std::size_t k = 0; k != N; ++k)
   {
      double mag = std::hypot(d[2*k], d[2*k+1]);
      double expected = (k == k0 || k == N - k0) ? double(N) / 2 : 0.0;
      REQUIRE_THAT(mag, Catch::Matchers::WithinAbs(expected, 1e-9));
   }
}

TEMPLATE_TEST_CASE_SIG("ifft inverts fft (round trip)", "[fft][ifft]",
   ((std::size_t N), N), 2, 4, 8, 16, 32, 64)
{
   auto d = make_signal<N>();
   auto orig = d;
   q::fft<N>(d.data());
   q::ifft<N>(d.data());
   for (std::size_t i = 0; i != 2*N; ++i)
      REQUIRE_THAT(d[i], Catch::Matchers::WithinAbs(orig[i], 1e-9));
}

TEST_CASE("fft: an impulse produces a flat spectrum", "[fft]")
{
   constexpr std::size_t N = 16;
   std::array<double, 2*N> d{};
   d[0] = 1.0;                                  // unit impulse at n = 0
   q::fft<N>(d.data());
   for (std::size_t k = 0; k != N; ++k)
   {
      REQUIRE_THAT(d[2*k],   Catch::Matchers::WithinAbs(1.0, eps));
      REQUIRE_THAT(d[2*k+1], Catch::Matchers::WithinAbs(0.0, eps));
   }
}

TEMPLATE_TEST_CASE_SIG("magspec equals |fft| for a real signal", "[fft][magspec]",
   ((std::size_t N), N), 8, 16, 32, 64)
{
   // Real input (imaginary slots zero).
   std::array<double, 2*N> d{};
   for (std::size_t n = 0; n != N; ++n)
      d[2*n] = std::cos(2.0 * q::pi * 3 * n / double(N))
             + 0.5 * std::sin(2.0 * q::pi * 7 * n / double(N));
   auto ref = d;

   // Reference magnitudes from a plain fft.
   q::fft<N>(ref.data());
   std::array<double, N/2 + 1> expected{};
   for (std::size_t k = 0; k <= N/2; ++k)
      expected[k] = std::hypot(ref[2*k], ref[2*k+1]);

   q::magspec<N>(d.data());
   for (std::size_t k = 0; k <= N/2; ++k)
      REQUIRE_THAT(d[k], Catch::Matchers::WithinAbs(expected[k], 1e-9));
}

TEST_CASE("magspec: DC and Nyquist magnitudes", "[fft][magspec]")
{
   constexpr std::size_t N = 16;
   SECTION("DC: a constant signal has all energy in bin 0")
   {
      std::array<double, 2*N> d{};
      for (std::size_t n = 0; n != N; ++n)
         d[2*n] = 1.0;
      q::magspec<N>(d.data());
      REQUIRE_THAT(d[0], Catch::Matchers::WithinAbs(double(N), eps));
      for (std::size_t k = 1; k <= N/2; ++k)
         REQUIRE_THAT(d[k], Catch::Matchers::WithinAbs(0.0, eps));
   }
   SECTION("Nyquist: an alternating signal has all energy in bin N/2")
   {
      std::array<double, 2*N> d{};
      for (std::size_t n = 0; n != N; ++n)
         d[2*n] = (n % 2 == 0) ? 1.0 : -1.0;
      q::magspec<N>(d.data());
      REQUIRE_THAT(d[N/2], Catch::Matchers::WithinAbs(double(N), eps));
      for (std::size_t k = 0; k != N/2; ++k)
         REQUIRE_THAT(d[k], Catch::Matchers::WithinAbs(0.0, eps));
   }
}
