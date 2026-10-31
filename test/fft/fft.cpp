/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fft/fft.hpp>
#include <q/support/literals.hpp>

#include <algorithm>
#include <array>
#include <complex>
#include <cmath>
#include <type_traits>
#include <vector>

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

// The float transforms of 16 points (complex) or 32 points (real) and up
// run on PFFFT when Q_USE_PFFFT is defined. fft_native.cpp builds this
// same file without it, so both backends face the same tests.

namespace
{
   // Naive O(N^2) DFT of a real signal, bins 0 .. N/2.
   template <typename T>
   std::vector<std::complex<double>> naive_real_dft(std::vector<T> const& x)
   {
      auto n = x.size();
      std::vector<std::complex<double>> out(n/2 + 1);
      for (std::size_t k = 0; k <= n/2; ++k)
      {
         std::complex<double> acc{};
         for (std::size_t t = 0; t != n; ++t)
         {
            double a = -2.0 * q::pi * double(k) * double(t) / double(n);
            acc += double(x[t]) * std::polar(1.0, a);
         }
         out[k] = acc;
      }
      return out;
   }

   // A deterministic real signal of n samples, with room for n+2.
   template <typename T>
   std::vector<T> make_real_signal(std::size_t n)
   {
      std::vector<T> x(n + 2);
      for (std::size_t t = 0; t != n; ++t)
         x[t] = T(std::sin(0.30 * t) + 0.5 * std::cos(0.011 * t * t)
              + 0.25 * ((t * 7919) % 101) / 101.0);
      return x;
   }

   // The largest |a[k] - b[k]| over the largest |b[k]|.
   double rel_err(
      std::vector<std::complex<double>> const& a
    , std::vector<std::complex<double>> const& b)
   {
      double peak = 0, err = 0;
      for (std::size_t k = 0; k != b.size(); ++k)
      {
         peak = std::max(peak, std::abs(b[k]));
         err = std::max(err, std::abs(a[k] - b[k]));
      }
      return err / peak;
   }

   template <typename T>
   std::vector<std::complex<double>> bins(T const* d, std::size_t count)
   {
      std::vector<std::complex<double>> out(count);
      for (std::size_t k = 0; k != count; ++k)
         out[k] = {double(d[2*k]), double(d[2*k+1])};
      return out;
   }

   template <typename T>
   constexpr double tolerance = std::is_same_v<T, float>? 1e-5 : 1e-12;
}

TEMPLATE_TEST_CASE_SIG("fft (float) matches the naive DFT", "[fft]",
   ((std::size_t N), N), 8, 16, 32, 64, 256, 1024, 4096)
{
   std::vector<float> d(2*N);
   for (std::size_t n = 0; n != N; ++n)
   {
      d[2*n] = float(std::sin(0.30 * n) + 0.5 * std::cos(0.011 * n * n));
      d[2*n+1] = float(0.25 * std::sin(0.70 * n) - 0.1 * n / double(N));
   }
   std::vector<std::complex<double>> ref(N);
   for (std::size_t k = 0; k != N; ++k)
   {
      for (std::size_t n = 0; n != N; ++n)
      {
         double a = -2.0 * q::pi * double(k) * double(n) / double(N);
         ref[k] += std::complex<double>(d[2*n], d[2*n+1])
            * std::complex<double>(std::cos(a), std::sin(a));
      }
   }

   q::fft<N>(d.data());
   REQUIRE(rel_err(bins(d.data(), N), ref) < 1e-5);
}

TEMPLATE_TEST_CASE_SIG("ifft inverts fft (float)", "[fft][ifft]",
   ((std::size_t N), N), 8, 16, 32, 64, 256, 1024, 4096)
{
   std::vector<float> d(2*N);
   for (std::size_t i = 0; i != 2*N; ++i)
      d[i] = float(std::sin(0.37 * i) + 0.2 * std::cos(0.013 * i * i));
   auto orig = d;

   q::fft<N>(d.data());
   q::ifft<N>(d.data());
   for (std::size_t i = 0; i != 2*N; ++i)
      REQUIRE_THAT(d[i], Catch::Matchers::WithinAbs(orig[i], 1e-5));
}

TEMPLATE_TEST_CASE_SIG("rfft matches the naive DFT", "[fft][rfft]",
   ((typename T, std::size_t N), T, N),
   (double, 2), (double, 4), (double, 8), (double, 16), (double, 64),
   (float, 2), (float, 4), (float, 16), (float, 32), (float, 64),
   (float, 256), (float, 1024), (float, 4096))
{
   auto x = make_real_signal<T>(N);
   auto ref = naive_real_dft(std::vector<T>(x.begin(), x.begin() + N));

   q::rfft<N>(x.data());
   REQUIRE(rel_err(bins(x.data(), N/2 + 1), ref) < tolerance<T>);
   REQUIRE(x[1] == T(0));
   REQUIRE(x[N+1] == T(0));
}

TEMPLATE_TEST_CASE_SIG("irfft inverts rfft (round trip)", "[fft][rfft]",
   ((typename T, std::size_t N), T, N),
   (double, 2), (double, 4), (double, 8), (double, 64),
   (float, 2), (float, 16), (float, 32), (float, 256), (float, 4096))
{
   auto x = make_real_signal<T>(N);
   auto orig = x;

   q::rfft<N>(x.data());
   q::irfft<N>(x.data());
   for (std::size_t t = 0; t != N; ++t)
      REQUIRE_THAT(x[t], Catch::Matchers::WithinAbs(orig[t], tolerance<T>));
}

TEMPLATE_TEST_CASE_SIG("rfft agrees with fft on a real signal", "[fft][rfft]",
   ((std::size_t N), N), 32, 1024)
{
   auto x = make_real_signal<float>(N);
   std::vector<float> c(2*N);
   for (std::size_t t = 0; t != N; ++t)
      c[2*t] = x[t];

   q::rfft<N>(x.data());
   q::fft<N>(c.data());
   REQUIRE(rel_err(bins(x.data(), N/2 + 1), bins(c.data(), N/2 + 1)) < 1e-5);
}

TEST_CASE("irfft ignores the imaginary parts of DC and Nyquist", "[fft][rfft]")
{
   constexpr std::size_t N = 64;
   auto a = make_real_signal<float>(N);
   q::rfft<N>(a.data());
   auto b = a;
   b[1] = 3.0f;
   b[N+1] = -2.0f;

   q::irfft<N>(a.data());
   q::irfft<N>(b.data());
   for (std::size_t t = 0; t != N; ++t)
      REQUIRE(a[t] == b[t]);
}

TEST_CASE("float transforms work on a misaligned buffer", "[fft][rfft]")
{
   // One float past a 16-byte boundary: PFFFT on x86 needs its SIMD
   // alignment, so this case goes through the aligned copy.
   constexpr std::size_t N = 256;
   alignas(16) float base[2*N + 4] = {};
   float* p = base + 1;

   std::vector<float> ref(2*N);
   for (std::size_t i = 0; i != 2*N; ++i)
      p[i] = ref[i] = float(std::sin(0.37 * i));

   q::fft<N>(ref.data());
   q::fft<N>(p);
   for (std::size_t i = 0; i != 2*N; ++i)
      REQUIRE(p[i] == ref[i]);

   q::ifft<N>(ref.data());
   q::ifft<N>(p);
   for (std::size_t i = 0; i != 2*N; ++i)
      REQUIRE(p[i] == ref[i]);

   std::vector<float> rref(N + 2);
   for (std::size_t i = 0; i != N; ++i)
      p[i] = rref[i] = float(std::cos(0.21 * i));

   q::rfft<N>(rref.data());
   q::rfft<N>(p);
   for (std::size_t i = 0; i != N + 2; ++i)
      REQUIRE(p[i] == rref[i]);

   q::irfft<N>(rref.data());
   q::irfft<N>(p);
   for (std::size_t i = 0; i != N; ++i)
      REQUIRE(p[i] == rref[i]);
}
