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
   constexpr auto pi = 3.14159265358979323846;

   // Spelled out rather than generated from detail::binomial, so the test
   // does not check the implementation against itself.
   constexpr std::array<int, 2> w2 = { 1, 1 };
   constexpr std::array<int, 3> w3 = { 1, 2, 1 };
   constexpr std::array<int, 4> w4 = { 1, 3, 3, 1 };
   constexpr std::array<int, 5> w5 = { 1, 4, 6, 4, 1 };

   // Every width emits, for output k, the causal convolution of the input
   // with its kernel evaluated at input index 2k+1:
   //
   //    y(k) = sum_j w[j] * x(2k+1-j) / scale,   x(n) = 0 for n < 0
   //
   // Written as a direct windowed sum, deliberately unlike the rotating
   // state the class uses, so that agreement means something.
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

   template <std::size_t N>
   constexpr int scale_of() { return 1 << (N-1); }

   ///////////////////////////////////////////////////////////////////////////
   // Agreement with a direct convolution, from the very first output: the
   // startup samples, where the kernel still hangs over the zero history,
   // are where a mis-rotated state shows up.
   ///////////////////////////////////////////////////////////////////////////
   template <std::size_t N>
   void check_matches_convolution(std::array<int, N> const& w)
   {
      std::vector<std::vector<double>> const signals =
         {ramp<double>(41), pseudo_random<double>(41)};

      for (auto const& x : signals)
      {
         auto got = run(q::basic_fast_downsample<N, double>{}, x);
         auto ref = convolve_ref(x, w, scale_of<N>());

         REQUIRE(got.size() == ref.size());
         REQUIRE(got.size() > N);            // past the zero-history startup
         for (std::size_t k = 0; k != got.size(); ++k)
            CHECK(got[k] == Approx(ref[k]).margin(1e-12));
      }
   }

   ///////////////////////////////////////////////////////////////////////////
   // Integer instantiations are bit-exact: the class sums then divides once,
   // and so does the reference.
   ///////////////////////////////////////////////////////////////////////////
   template <std::size_t N>
   void check_integer_is_exact(std::array<int, N> const& w)
   {
      auto x = pseudo_random<int>(41);
      auto got = run(q::basic_fast_downsample<N, int>{}, x);
      auto ref = convolve_ref(x, w, scale_of<N>());

      REQUIRE(got.size() == ref.size());
      for (std::size_t k = 0; k != got.size(); ++k)
         CHECK(got[k] == ref[k]);
   }

   ///////////////////////////////////////////////////////////////////////////
   // DC gain is exactly 1.0 once the window clears the zero history, and the
   // outputs before that are the partial sums, not garbage. Output k sees
   // taps j = 0 .. min(N-1, 2k+1).
   ///////////////////////////////////////////////////////////////////////////
   template <std::size_t N>
   void check_dc_gain(std::array<int, N> const& w)
   {
      constexpr double level = 0.375;
      std::vector<double> x(40, level);
      auto y = run(q::basic_fast_downsample<N, double>{}, x);

      for (std::size_t k = 0; k != y.size(); ++k)
      {
         int partial = 0;
         for (std::size_t j = 0; j != N && int(j) <= int((2*k)+1); ++j)
            partial += w[j];
         CHECK(y[k] == Approx(level * partial / scale_of<N>()).margin(1e-15));
      }
      CHECK(y.back() == Approx(level).margin(1e-15));   // fully inside by the end
   }

   ///////////////////////////////////////////////////////////////////////////
   // Recover the impulse response from the class itself and check it against
   // the documented kernel and frequency response. An impulse at input index
   // n0 gives y(k) = h(2k+1-n0), so n0 = 0 and n0 = 1 together yield every
   // tap.
   ///////////////////////////////////////////////////////////////////////////
   template <std::size_t N>
   std::array<double, N> impulse_response()
   {
      std::array<double, N> h = {};
      for (int n0 = 0; n0 != 2; ++n0)
      {
         std::vector<double> x(4*N, 0.0);
         x[n0] = 1.0;
         auto y = run(q::basic_fast_downsample<N, double>{}, x);
         for (std::size_t k = 0; k != y.size(); ++k)
         {
            int j = int((2*k)+1) - n0;
            if (j >= 0 && j < int(N))
               h[j] = y[k];
         }
      }
      return h;
   }

   template <std::size_t N>
   double response_db(std::array<double, N> const& h, double w)
   {
      std::complex<double> acc = 0.0;
      for (std::size_t j = 0; j != N; ++j)
         acc += h[j] * std::exp(std::complex<double>(0.0, -w * double(j)));
      return 20.0 * std::log10(std::abs(acc));
   }

   template <std::size_t N>
   void check_impulse_response(std::array<int, N> const& w)
   {
      auto h = impulse_response<N>();

      for (std::size_t j = 0; j != N; ++j)
         CHECK(h[j] == Approx(double(w[j]) / scale_of<N>()).margin(1e-15));

      // The documented response is cos^(N-1)(w/2).
      CHECK(response_db(h, 0.0) == Approx(0.0).margin(1e-12));
      for (auto wr : {pi/2, 0.8*pi})
      {
         auto want = 20.0 * std::log10(std::pow(std::cos(wr/2), double(N-1)));
         CHECK(response_db(h, wr) == Approx(want).margin(1e-9));
      }
   }

   ///////////////////////////////////////////////////////////////////////////
   // n cascaded stages collapse exactly to a CIC of rate R = 2^n and order
   // N-1, because prod(k = 0..n-1)(1 + z^-2^k) is a length-2^n boxcar. Check
   // four stages against a length-16 boxcar raised to that order. Composing
   // the per-stage alignment, cascade output k lands at input index 16k+15.
   ///////////////////////////////////////////////////////////////////////////
   std::vector<double> convolve(
      std::vector<double> const& a, std::vector<double> const& b)
   {
      std::vector<double> out(a.size() + b.size() - 1, 0.0);
      for (std::size_t i = 0; i != a.size(); ++i)
         for (std::size_t j = 0; j != b.size(); ++j)
            out[i+j] += a[i] * b[j];
      return out;
   }

   template <std::size_t N>
   void check_cascade_is_cic()
   {
      using DS = q::basic_fast_downsample<N, double>;
      auto x = pseudo_random<double>(400);

      auto y = run(DS{}, x);
      y = run(DS{}, y);
      y = run(DS{}, y);
      y = run(DS{}, y);

      std::vector<double> h = {1.0};                  // boxcar(16) ^ (N-1)
      std::vector<double> const box(16, 1.0/16.0);
      for (std::size_t i = 0; i != N-1; ++i)
         h = convolve(h, box);

      REQUIRE(y.size() > 4);
      for (std::size_t k = 0; k != y.size(); ++k)
      {
         double acc = 0.0;
         for (std::size_t j = 0; j != h.size(); ++j)
         {
            int n = int((16*k)+15) - int(j);
            if (n >= 0 && n < int(x.size()))
               acc += h[j] * x[n];
         }
         CHECK(y[k] == Approx(acc).margin(1e-12));
      }
   }

   template <std::size_t N>
   void check_width(std::array<int, N> const& w)
   {
      check_matches_convolution<N>(w);
      check_integer_is_exact<N>(w);
      check_dc_gain<N>(w);
      check_impulse_response<N>(w);
      check_cascade_is_cic<N>();
   }
}

TEST_CASE("Test_fast_downsample_2")  { check_width<2>(w2); }
TEST_CASE("Test_fast_downsample_3")  { check_width<3>(w3); }
TEST_CASE("Test_fast_downsample_4")  { check_width<4>(w4); }
TEST_CASE("Test_fast_downsample_5")  { check_width<5>(w5); }

///////////////////////////////////////////////////////////////////////////////
// The named widths are aliases of the generic template, and fast_downsample
// is the original name of the three-tap one. Code spelling any of them must
// still compile and behave.
///////////////////////////////////////////////////////////////////////////////
static_assert(std::is_same_v<q::fast_downsample_2<float>, q::basic_fast_downsample<2, float>>);
static_assert(std::is_same_v<q::fast_downsample_3<float>, q::basic_fast_downsample<3, float>>);
static_assert(std::is_same_v<q::fast_downsample_4<float>, q::basic_fast_downsample<4, float>>);
static_assert(std::is_same_v<q::fast_downsample_5<float>, q::basic_fast_downsample<5, float>>);
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

///////////////////////////////////////////////////////////////////////////////
// fast_downsample_2 needs no state at all: its window is exactly the pair it
// is handed.
///////////////////////////////////////////////////////////////////////////////
static_assert(q::basic_fast_downsample<2, float>{}.x.empty(),
   "fast_downsample_2 must be stateless");

///////////////////////////////////////////////////////////////////////////////
// The call operator is constexpr, so a whole run must fold at compile time.
///////////////////////////////////////////////////////////////////////////////
namespace
{
   template <std::size_t N>
   constexpr double constexpr_run()
   {
      q::basic_fast_downsample<N, double> f{};
      f(1.0, 1.0);
      f(1.0, 1.0);
      return f(1.0, 1.0);      // window full: DC gain 1
   }
}

static_assert(constexpr_run<2>() == 1.0);
static_assert(constexpr_run<3>() == 1.0);
static_assert(constexpr_run<4>() == 1.0);
static_assert(constexpr_run<5>() == 1.0);
