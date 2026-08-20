/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/fast_downsample.hpp>

#include <array>
#include <cmath>
#include <complex>
#include <type_traits>
#include <vector>

namespace q = cycfi::q;

namespace
{
   // Both downsamplers emit, for output k, the causal convolution of the
   // input with their kernel evaluated at input index 2k+1:
   //
   //    y(k) = sum_j w[j] * x(2k+1-j) / scale,   x(n) = 0 for n < 0
   //
   // This is written as a direct windowed sum, deliberately unlike the
   // rotating state the classes use, so that agreement means something.
   template <typename T, std::size_t N>
   std::vector<T> convolve_ref(
      std::vector<T> const& x, std::array<int, N> const& w, int scale)
   {
      std::vector<T> y;
      for (std::size_t k = 0; (2*k)+1 < x.size(); ++k)
      {
         T acc = T(0);
         for (std::size_t j = 0; j != N; ++j)
         {
            int n = int((2*k)+1) - int(j);
            if (n >= 0)
               acc += T(w[j]) * x[n];
         }
         y.push_back(acc / T(scale));
      }
      return y;
   }

   template <typename F, typename T>
   std::vector<T> run(F&& f, std::vector<T> const& x)
   {
      std::vector<T> y;
      for (std::size_t i = 0; (i+1) < x.size(); i += 2)
         y.push_back(f(x[i], x[i+1]));
      return y;
   }

   // A ramp exercises the coefficients only weakly (any kernel with the same
   // first moment agrees on it), so pair it with a signal that has no such
   // symmetry to hide behind.
   template <typename T>
   std::vector<T> ramp(std::size_t n)
   {
      std::vector<T> x;
      for (std::size_t i = 0; i != n; ++i)
         x.push_back(T(i));
      return x;
   }

   template <typename T>
   std::vector<T> pseudo_random(std::size_t n)
   {
      std::vector<T> x;
      for (std::size_t i = 0; i != n; ++i)
         x.push_back(T(int((i*7) % 13) - 6) + T(int((i*5) % 3)));
      return x;
   }

   constexpr std::array<int, 3> w3 = { 1, 2, 1 };
   constexpr std::array<int, 5> w5 = { 1, 4, 6, 4, 1 };
}

///////////////////////////////////////////////////////////////////////////////
// Agreement with a direct convolution, from the very first output: the
// startup samples, where the kernel still hangs over the zero history, are
// where a mis-rotated state shows up.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test_fast_downsample_5_matches_convolution")
{
   std::vector<std::vector<double>> const signals =
      {ramp<double>(41), pseudo_random<double>(41)};

   for (auto const& x : signals)
   {
      auto got = run(q::fast_downsample_5<double>{}, x);
      auto ref = convolve_ref(x, w5, 16);

      REQUIRE(got.size() == ref.size());
      REQUIRE(got.size() > 4);            // past the zero-history startup
      for (std::size_t k = 0; k != got.size(); ++k)
         CHECK(got[k] == Approx(ref[k]).margin(1e-12));
   }
}

TEST_CASE("Test_fast_downsample_3_matches_convolution")
{
   std::vector<std::vector<double>> const signals =
      {ramp<double>(41), pseudo_random<double>(41)};

   for (auto const& x : signals)
   {
      auto got = run(q::fast_downsample_3<double>{}, x);
      auto ref = convolve_ref(x, w3, 4);

      REQUIRE(got.size() == ref.size());
      for (std::size_t k = 0; k != got.size(); ++k)
         CHECK(got[k] == Approx(ref[k]).margin(1e-12));
   }
}

///////////////////////////////////////////////////////////////////////////////
// Integer instantiations are bit-exact against the same reference, provided
// the reference divides where the class divides. fast_downsample_3 divides its
// inputs before summing, so it is not exact against a sum-then-divide
// reference; fast_downsample_5 sums first and is.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test_fast_downsample_5_integer_is_exact")
{
   auto x = pseudo_random<int>(41);
   auto got = run(q::fast_downsample_5<int>{}, x);
   auto ref = convolve_ref(x, w5, 16);

   REQUIRE(got.size() == ref.size());
   for (std::size_t k = 0; k != got.size(); ++k)
      CHECK(got[k] == ref[k]);
}

///////////////////////////////////////////////////////////////////////////////
// DC gain is exactly 1.0. Once the window is full the output must equal the
// input level, not merely approach it.
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test_fast_downsample_5_dc_gain")
{
   constexpr double level = 0.375;
   std::vector<double> x(40, level);

   auto y = run(q::fast_downsample_5<double>{}, x);
   REQUIRE(y.size() == 20);

   // The window spans 5 input samples, so output 2 onwards is fully inside.
   for (std::size_t k = 2; k != y.size(); ++k)
      CHECK(y[k] == Approx(level).margin(1e-15));

   // ... and the startup outputs are the partial sums, not garbage.
   CHECK(y[0] == Approx(level * 5.0/16.0).margin(1e-15));
   CHECK(y[1] == Approx(level * 15.0/16.0).margin(1e-15));
}

///////////////////////////////////////////////////////////////////////////////
// Recover the impulse response from the class itself and check it against the
// documented kernel and frequency response. An impulse at input index n0
// produces y(k) = h(2k+1-n0), so n0 = 0 and n0 = 1 together yield every tap.
///////////////////////////////////////////////////////////////////////////////
namespace
{
   template <typename F>
   std::array<double, 5> impulse_response()
   {
      std::array<double, 5> h = {};
      for (int n0 = 0; n0 != 2; ++n0)
      {
         std::vector<double> x(16, 0.0);
         x[n0] = 1.0;
         auto y = run(F{}, x);
         for (std::size_t k = 0; k != y.size(); ++k)
         {
            int j = int((2*k)+1) - n0;
            if (j >= 0 && j < 5)
               h[j] = y[k];
         }
      }
      return h;
   }

   double response_db(std::array<double, 5> const& h, double w)
   {
      std::complex<double> acc = 0.0;
      for (std::size_t j = 0; j != h.size(); ++j)
         acc += h[j] * std::exp(std::complex<double>(0.0, -w * double(j)));
      return 20.0 * std::log10(std::abs(acc));
   }
}

TEST_CASE("Test_fast_downsample_5_impulse_response")
{
   auto h = impulse_response<q::fast_downsample_5<double>>();

   for (std::size_t j = 0; j != h.size(); ++j)
      CHECK(h[j] == Approx(double(w5[j]) / 16.0).margin(1e-15));

   // The magnitude response documented in the header: cos^4(w/2).
   constexpr auto pi = 3.14159265358979323846;
   CHECK(response_db(h, pi/2)     == Approx(-12.04).margin(0.01));  // fs/4
   CHECK(response_db(h, 0.8*pi)   == Approx(-40.80).margin(0.01));  // 0.40 fs
   CHECK(response_db(h, 0.0)      == Approx(0.0).margin(1e-12));    // DC
}

///////////////////////////////////////////////////////////////////////////////
// The call operator is constexpr, so a whole run must fold at compile time.
///////////////////////////////////////////////////////////////////////////////
namespace
{
   constexpr double constexpr_run()
   {
      q::fast_downsample_5<double> f{};
      f(1.0, 1.0);
      f(1.0, 1.0);
      return f(1.0, 1.0);      // window full: DC gain 1
   }
}

static_assert(constexpr_run() == 1.0, "fast_downsample_5 must be usable in a constant expression");

///////////////////////////////////////////////////////////////////////////////
// fast_downsample is the original name for fast_downsample_3, kept as an alias.
// Existing code spelling it the old way must still compile and still behave.
///////////////////////////////////////////////////////////////////////////////
static_assert(std::is_same_v<q::fast_downsample<float>, q::fast_downsample_3<float>>,
   "fast_downsample must remain an alias for fast_downsample_3");

TEST_CASE("Test_fast_downsample_alias")
{
   auto x = pseudo_random<double>(41);
   auto via_alias = run(q::fast_downsample<double>{}, x);
   auto ref = convolve_ref(x, w3, 4);

   REQUIRE(via_alias.size() == ref.size());
   for (std::size_t k = 0; k != via_alias.size(); ++k)
      CHECK(via_alias[k] == Approx(ref[k]).margin(1e-12));
}
