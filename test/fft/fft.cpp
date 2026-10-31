/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fft/fft.hpp>
#include <q/support/base.hpp>

#include <algorithm>
#include <cmath>
#include <complex>
#include <type_traits>
#include <utility>
#include <vector>

namespace q = cycfi::q;
using Catch::Matchers::WithinAbs;

// fft works in place on n interleaved complex values (2n scalars), rfft on
// n real samples in n+2 scalars, giving the n/2+1 bins from DC to Nyquist.
// With Q_USE_PFFFT, float transforms of 16 points (fft) or 32 (rfft) and up
// run on PFFFT. fft_native.cpp builds this same file without it, so both
// implementations face the same tests.

namespace
{
   template <typename T>
   constexpr double tolerance = std::is_same_v<T, float>? 1e-5 : 1e-11;

   // A deterministic signal of n scalars.
   template <typename T>
   std::vector<T> make_signal(std::size_t n)
   {
      std::vector<T> x(n);
      for (std::size_t i = 0; i != n; ++i)
         x[i] = T(std::sin(0.37 * i) + 0.2 * std::cos(0.013 * i * i)
              + 0.25 * ((i * 7919) % 101) / 101.0);
      return x;
   }

   // Naive O(n^2) DFT of n complex values, interleaved.
   template <typename T>
   std::vector<std::complex<double>> naive_dft(std::vector<T> const& x)
   {
      auto n = x.size() / 2;
      std::vector<std::complex<double>> out(n);
      for (std::size_t k = 0; k != n; ++k)
      {
         for (std::size_t t = 0; t != n; ++t)
         {
            double a = -2.0 * q::pi * double(k) * double(t) / double(n);
            out[k] += std::complex<double>(x[2*t], x[2*t+1])
               * std::polar(1.0, a);
         }
      }
      return out;
   }

   // Naive O(n^2) DFT of n real samples, bins 0 .. n/2.
   template <typename T>
   std::vector<std::complex<double>>
   naive_real_dft(std::vector<T> const& x, std::size_t n)
   {
      std::vector<std::complex<double>> out(n/2 + 1);
      for (std::size_t k = 0; k <= n/2; ++k)
      {
         for (std::size_t t = 0; t != n; ++t)
         {
            double a = -2.0 * q::pi * double(k) * double(t) / double(n);
            out[k] += double(x[t]) * std::polar(1.0, a);
         }
      }
      return out;
   }

   template <typename T>
   std::vector<std::complex<double>> bins(T const* d, std::size_t count)
   {
      std::vector<std::complex<double>> out(count);
      for (std::size_t k = 0; k != count; ++k)
         out[k] = {double(d[2*k]), double(d[2*k+1])};
      return out;
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
}

TEMPLATE_TEST_CASE("fft matches the naive DFT", "[fft]", float, double)
{
   using T = TestType;
   for (std::size_t n : {1, 2, 4, 8, 16, 32, 256, 1024, 4096})
   {
      auto d = make_signal<T>(2*n);
      auto ref = naive_dft(d);
      q::basic_fft<T> f{n};
      f.forward(d.data());
      REQUIRE(f.size() == n);
      REQUIRE(rel_err(bins(d.data(), n), ref) < tolerance<T>);
   }
}

TEMPLATE_TEST_CASE("fft: inverse undoes forward", "[fft]", float, double)
{
   using T = TestType;
   for (std::size_t n : {1, 2, 8, 16, 64, 4096})
   {
      auto d = make_signal<T>(2*n);
      auto orig = d;
      q::basic_fft<T> f{n};
      f.forward(d.data());
      f.inverse(d.data());
      for (std::size_t i = 0; i != 2*n; ++i)
         REQUIRE_THAT(d[i], WithinAbs(orig[i], tolerance<T>));
   }
}

TEST_CASE("fft: a complex exponential maps to a single bin", "[fft]")
{
   constexpr std::size_t n = 32;
   constexpr std::size_t k0 = 5;
   std::vector<double> d(2*n);
   for (std::size_t t = 0; t != n; ++t)
   {
      double a = 2.0 * q::pi * k0 * t / double(n);
      d[2*t] = std::cos(a);
      d[2*t+1] = std::sin(a);
   }
   q::basic_fft<double> f{n};
   f.forward(d.data());
   for (std::size_t k = 0; k != n; ++k)
   {
      double mag = std::hypot(d[2*k], d[2*k+1]);
      REQUIRE_THAT(mag, WithinAbs(k == k0? double(n) : 0.0, 1e-9));
   }
}

TEST_CASE("fft: an impulse produces a flat spectrum", "[fft]")
{
   constexpr std::size_t n = 16;
   std::vector<double> d(2*n);
   d[0] = 1.0;
   q::basic_fft<double> f{n};
   f.forward(d.data());
   for (std::size_t k = 0; k != n; ++k)
   {
      REQUIRE_THAT(d[2*k], WithinAbs(1.0, 1e-12));
      REQUIRE_THAT(d[2*k+1], WithinAbs(0.0, 1e-12));
   }
}

TEMPLATE_TEST_CASE("rfft matches the naive DFT", "[rfft]", float, double)
{
   using T = TestType;
   for (std::size_t n : {2, 4, 8, 16, 32, 64, 256, 1024, 4096})
   {
      auto x = make_signal<T>(n + 2);
      auto ref = naive_real_dft(x, n);
      q::basic_rfft<T> f{n};
      f.forward(x.data());
      REQUIRE(f.size() == n);
      REQUIRE(rel_err(bins(x.data(), n/2 + 1), ref) < tolerance<T>);
      REQUIRE(x[1] == T(0));
      REQUIRE(x[n+1] == T(0));
   }
}

TEMPLATE_TEST_CASE("rfft: inverse undoes forward", "[rfft]", float, double)
{
   using T = TestType;
   for (std::size_t n : {2, 4, 16, 32, 256, 4096})
   {
      auto x = make_signal<T>(n + 2);
      auto orig = x;
      q::basic_rfft<T> f{n};
      f.forward(x.data());
      f.inverse(x.data());
      for (std::size_t t = 0; t != n; ++t)
         REQUIRE_THAT(x[t], WithinAbs(orig[t], tolerance<T>));
   }
}

TEST_CASE("rfft agrees with fft on a real signal", "[rfft]")
{
   for (std::size_t n : {32, 1024})
   {
      auto x = make_signal<float>(n + 2);
      std::vector<float> c(2*n);
      for (std::size_t t = 0; t != n; ++t)
         c[2*t] = x[t];

      q::rfft r{n};
      q::fft f{n};
      r.forward(x.data());
      f.forward(c.data());
      REQUIRE(rel_err(bins(x.data(), n/2 + 1), bins(c.data(), n/2 + 1))
         < 1e-5);
   }
}

TEST_CASE("rfft: inverse ignores the imaginary parts of DC and Nyquist",
   "[rfft]")
{
   for (std::size_t n : {8, 64})
   {
      q::rfft f{n};
      auto a = make_signal<float>(n + 2);
      f.forward(a.data());
      auto b = a;
      b[1] = 3.0f;
      b[n+1] = -2.0f;

      f.inverse(a.data());
      f.inverse(b.data());
      for (std::size_t t = 0; t != n; ++t)
         REQUIRE(a[t] == b[t]);
   }
}

TEST_CASE("float transforms work on a misaligned buffer", "[fft][rfft]")
{
   // One float past a 16-byte boundary: PFFFT on x86 needs its SIMD
   // alignment, so this case goes through the object's aligned copy.
   constexpr std::size_t n = 256;
   alignas(16) float base[2*n + 4] = {};
   float* p = base + 1;

   auto ref = make_signal<float>(2*n);
   std::copy(ref.begin(), ref.end(), p);
   q::fft f{n};
   f.forward(ref.data());
   f.forward(p);
   REQUIRE(std::equal(ref.begin(), ref.end(), p));
   f.inverse(ref.data());
   f.inverse(p);
   REQUIRE(std::equal(ref.begin(), ref.end(), p));

   auto rref = make_signal<float>(n + 2);
   std::copy(rref.begin(), rref.end(), p);
   q::rfft r{n};
   r.forward(rref.data());
   r.forward(p);
   REQUIRE(std::equal(rref.begin(), rref.end(), p));
   r.inverse(rref.data());
   r.inverse(p);
   REQUIRE(std::equal(rref.begin(), rref.begin() + n, p));
}

TEST_CASE("fft and rfft copy and move", "[fft][rfft]")
{
   constexpr std::size_t n = 1024;
   auto x = make_signal<float>(n + 2);

   q::rfft a{n};
   auto ref = x;
   a.forward(ref.data());

   q::rfft copied{a};
   q::rfft assigned{64};
   assigned = a;
   q::rfft moved{std::move(copied)};

   for (auto* f : {&a, &assigned, &moved})
   {
      auto y = x;
      f->forward(y.data());
      REQUIRE(y == ref);
      REQUIRE(f->size() == n);
   }
}
