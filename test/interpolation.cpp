/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/utility/fractional_ring_buffer.hpp>

#include <cmath>
#include <vector>

namespace q = cycfi::q;
using namespace q::sample_interpolation;

namespace
{
   // Fill a buffer with a ramp: push 0, 1, 2, ... n-1. Since index 0 is the
   // newest element, the value at (fractional) index i is (n-1) - i: a
   // perfectly linear function of the index.
   template <typename Buffer>
   void fill_ramp(Buffer& buf, int n)
   {
      for (auto i = 0; i != n; ++i)
         buf.push(float(i));
   }
}

TEST_CASE("none: truncating lookup")
{
   q::fractional_ring_buffer<
      float, std::vector<float>, float, none> buf(4);

   for (float v : {10.0f, 20.0f, 30.0f, 40.0f})
      buf.push(v);

   // Index 0 is the newest element
   CHECK(buf[0.0f] == 40.0f);
   CHECK(buf[1.0f] == 30.0f);
   CHECK(buf[2.0f] == 20.0f);
   CHECK(buf[3.0f] == 10.0f);

   // The fractional part is truncated, not rounded
   CHECK(buf[0.5f] == 40.0f);
   CHECK(buf[0.99f] == 40.0f);
   CHECK(buf[2.5f] == 20.0f);
}

TEST_CASE("linear: endpoints and midpoints")
{
   q::fractional_ring_buffer<float> buf(4);   // linear is the default

   for (float v : {10.0f, 20.0f, 30.0f, 40.0f})
      buf.push(v);

   // At integer indices, the sample itself
   CHECK(buf[0.0f] == Approx(40.0f));
   CHECK(buf[1.0f] == Approx(30.0f));
   CHECK(buf[2.0f] == Approx(20.0f));

   // Halfway between adjacent samples
   CHECK(buf[0.5f] == Approx(35.0f));
   CHECK(buf[1.5f] == Approx(25.0f));
   CHECK(buf[2.5f] == Approx(15.0f));
}

TEST_CASE("linear: exact on a ramp")
{
   q::fractional_ring_buffer<float> buf(8);
   fill_ramp(buf, 8);

   // Linear interpolation reproduces a linear signal exactly. Valid index
   // range for linear is [0, size-2]: index i reads samples i and i+1.
   for (float i = 0.0f; i <= 6.0f; i += 0.25f)
      CHECK(buf[i] == Approx(7.0f - i));
}

TEST_CASE("linear: reads are consistent across the ring wrap")
{
   q::fractional_ring_buffer<float> buf(8);

   // Push many more samples than the capacity so the write position wraps;
   // fractional reads must be seamless across the physical buffer boundary.
   fill_ramp(buf, 100);

   for (float i = 0.0f; i <= 6.0f; i += 0.5f)
      CHECK(buf[i] == Approx(99.0f - i));
}

TEST_CASE("cosine: endpoints, midpoint, constants")
{
   q::fractional_ring_buffer<
      float, std::vector<float>, float, cosine> buf(4);

   for (float v : {10.0f, 20.0f, 30.0f, 40.0f})
      buf.push(v);

   // Passes through the samples at integer indices
   CHECK(buf[0.0f] == Approx(40.0f));
   CHECK(buf[1.0f] == Approx(30.0f));
   CHECK(buf[2.0f] == Approx(20.0f));

   // The midpoint coincides with linear interpolation
   CHECK(buf[0.5f] == Approx(35.0f));
   CHECK(buf[1.5f] == Approx(25.0f));

   // Quarter-way, the cosine ease hugs y1 closer than linear would
   auto m = (1.0f - std::cos(0.25f * float(M_PI))) / 2;
   CHECK(buf[0.25f] == Approx(40.0f + m * (30.0f - 40.0f)));

   // Constants are reproduced exactly
   q::fractional_ring_buffer<
      float, std::vector<float>, float, cosine> cbuf(4);
   for (auto i = 0; i != 4; ++i)
      cbuf.push(7.5f);
   for (float i = 0.0f; i <= 2.0f; i += 0.25f)
      CHECK(cbuf[i] == Approx(7.5f));
}

TEST_CASE("cosine: tracks the analytic ease")
{
   // The implementation may use the sin lookup table; it must stay within
   // 1e-3 of the analytic raised-cosine ease over the whole segment.
   constexpr auto n = 8;
   q::fractional_ring_buffer<
      float, std::vector<float>, float, cosine> buf(n);

   for (float v : {3.0f, -1.0f, 4.0f, 1.0f, -5.0f, 9.0f, 2.0f, 6.0f})
      buf.push(v);

   for (float i = 0.0f; i <= float(n-2); i += 0.03125f)
   {
      auto k = std::floor(i);
      auto mu = i - k;
      auto m = (1.0 - std::cos(mu * M_PI)) / 2;
      auto y1 = buf[k];
      auto y2 = buf[k+1.0f];
      auto expected = y1 + m * (y2 - y1);
      CHECK(buf[i] == Approx(expected).margin(1e-3));
   }
}

TEST_CASE("hermite: endpoints pass through the samples")
{
   q::fractional_ring_buffer<
      float, std::vector<float>, float, hermite> buf(8);

   for (float v : {3.0f, -1.0f, 4.0f, 1.0f, -5.0f, 9.0f, 2.0f, 6.0f})
      buf.push(v);

   // 4-point types are valid for indices in [1, size-3]
   CHECK(buf[1.0f] == Approx(2.0f));
   CHECK(buf[2.0f] == Approx(9.0f));
   CHECK(buf[3.0f] == Approx(-5.0f));
   CHECK(buf[4.0f] == Approx(1.0f));
   CHECK(buf[5.0f] == Approx(4.0f));
}

TEST_CASE("hermite: exact on quadratics")
{
   constexpr auto n = 16;
   q::fractional_ring_buffer<
      float, std::vector<float>, float, hermite> buf(n);

   auto f = [](float x) { return 0.5f*x*x - 3.0f*x + 2.0f; };
   for (auto i = 0; i != n; ++i)
      buf.push(f(float(i)));

   // The value at index i is f(n-1 - i), also a quadratic in i. Catmull-Rom
   // tangents (central differences) are exact for quadratics, so the
   // interpolation must reproduce f over the whole valid range [1, n-3].
   for (float i = 1.0f; i <= float(n-3); i += 0.25f)
      CHECK(buf[i] == Approx(f(n-1 - i)).margin(1e-3));
}

TEST_CASE("cubic: exact on cubics")
{
   constexpr auto n = 16;
   q::fractional_ring_buffer<
      float, std::vector<float>, float, cubic> buf(n);

   auto f = [](float x) { return 0.02f*x*x*x - 0.3f*x*x + x + 1.0f; };
   for (auto i = 0; i != n; ++i)
      buf.push(f(float(i)));

   // 4-point Lagrange interpolation reproduces cubic polynomials exactly
   for (float i = 1.0f; i <= float(n-3); i += 0.25f)
      CHECK(buf[i] == Approx(f(n-1 - i)).margin(1e-3));
}

TEST_CASE("bspline: linear precision")
{
   q::fractional_ring_buffer<
      float, std::vector<float>, float, bspline> buf(16);
   fill_ramp(buf, 16);

   // The cubic B-spline is a smoother, but it has linear precision: ramps
   // are reproduced exactly.
   for (float i = 1.0f; i <= 13.0f; i += 0.25f)
      CHECK(buf[i] == Approx(15.0f - i).margin(1e-3));
}

TEST_CASE("bspline: smooths instead of passing through")
{
   constexpr auto n = 16;
   q::fractional_ring_buffer<
      float, std::vector<float>, float, bspline> buf(n);

   auto f = [](float x) { return 0.5f*x*x - 3.0f*x + 2.0f; };
   for (auto i = 0; i != n; ++i)
      buf.push(f(float(i)));

   // On a quadratic, the B-spline at an integer index yields
   // (y[i-1] + 4*y[i] + y[i+1]) / 6 = y[i] + (second difference)/6,
   // not y[i] itself: it trades pass-through for smoothness.
   auto second_diff = 1.0f;   // f'' = 1 everywhere for this f
   CHECK(buf[5.0f] == Approx(f(n-1 - 5) + second_diff/6).margin(1e-3));
   CHECK(buf[9.0f] == Approx(f(n-1 - 9) + second_diff/6).margin(1e-3));
}

TEST_CASE("linear vs none: fractional-delay error on a sine")
{
   // Characterize the error reading a sine at half-sample offsets. This
   // sets the quality baseline that higher-order interpolation types must
   // beat: for a sine of angular increment w, the worst-case linear
   // interpolation error is ~w^2/8, while truncation (none) is ~w/2.
   constexpr auto size = 512;
   constexpr auto sps = 48000.0;
   constexpr auto freq = 1000.0;
   auto const w = 2 * M_PI * freq / sps;

   q::fractional_ring_buffer<
      float, std::vector<float>, float, none> nbuf(size);
   q::fractional_ring_buffer<float> lbuf(size);

   for (auto i = 0; i != size; ++i)
   {
      auto s = float(std::sin(w * i));
      nbuf.push(s);
      lbuf.push(s);
   }

   // The value at fractional index i is sin(w * (size-1 - i))
   auto max_err_none = 0.0;
   auto max_err_linear = 0.0;
   for (auto k = 1; k != 100; ++k)
   {
      auto i = k + 0.5f;
      auto expected = std::sin(w * (size-1 - double(i)));
      max_err_none = std::max(
         max_err_none, std::abs(nbuf[i] - expected));
      max_err_linear = std::max(
         max_err_linear, std::abs(lbuf[i] - expected));
   }

   // Truncation is off by up to half a sample: error ~ sin'(t) * w/2
   CHECK(max_err_none < (w/2) * 1.1);
   CHECK(max_err_none > (w/2) * 0.5);    // sanity: it is this bad

   // Linear: worst-case error ~ w^2/8 at half-sample offsets
   CHECK(max_err_linear < (w*w/8) * 1.2);

   // And the whole point: linear is far better than truncation
   CHECK(max_err_linear < max_err_none / 10);
}

TEST_CASE("quality ladder: all types on the sine fractional-delay task")
{
   constexpr auto size = 512;
   constexpr auto sps = 48000.0;
   constexpr auto freq = 1000.0;
   auto const w = 2 * M_PI * freq / sps;

   q::fractional_ring_buffer<float, std::vector<float>, float, none>     b_none(size);
   q::fractional_ring_buffer<float, std::vector<float>, float, linear>   b_linear(size);
   q::fractional_ring_buffer<float, std::vector<float>, float, cosine>   b_cosine(size);
   q::fractional_ring_buffer<float, std::vector<float>, float, cubic>    b_cubic(size);
   q::fractional_ring_buffer<float, std::vector<float>, float, hermite>  b_hermite(size);
   q::fractional_ring_buffer<float, std::vector<float>, float, bspline>  b_bspline(size);

   for (auto i = 0; i != size; ++i)
   {
      auto s = float(std::sin(w * i));
      b_none.push(s);     b_linear.push(s);   b_cosine.push(s);
      b_cubic.push(s);    b_hermite.push(s);  b_bspline.push(s);
   }

   auto max_err = [&](auto& buf)
   {
      auto err = 0.0;
      for (auto k = 1; k != 100; ++k)
         for (auto frac : {0.25, 0.5, 0.75})
         {
            auto i = float(k + frac);
            auto expected = std::sin(w * (size-1 - double(i)));
            err = std::max(err, std::abs(double(buf[i]) - expected));
         }
      return err;
   };

   auto e_none     = max_err(b_none);
   auto e_linear   = max_err(b_linear);
   auto e_cosine   = max_err(b_cosine);
   auto e_cubic    = max_err(b_cubic);
   auto e_hermite  = max_err(b_hermite);
   auto e_bspline  = max_err(b_bspline);

   // The 4-point interpolators are the quality tier: at least an order of
   // magnitude better than linear on band-limited content.
   CHECK(e_hermite < e_linear / 10);
   CHECK(e_cubic   < e_linear / 10);
   CHECK(e_hermite < 1e-4);
   CHECK(e_cubic   < 1e-4);

   // The B-spline smoother attenuates highs (no pass-through), so it ranks
   // below linear here -- but far above truncation.
   CHECK(e_bspline < e_none / 10);

   // Honest accounting: cosine ease *distorts* in-band content (it is exact
   // only at mu = 0, 0.5, 1), so on a pure sine it is worse than linear.
   // Its virtue is the continuous first derivative at segment boundaries.
   CHECK(e_cosine > e_linear);
   CHECK(e_cosine < e_none);
}
