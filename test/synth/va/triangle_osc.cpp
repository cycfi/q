/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/synth/va/triangle_osc.hpp>
#include <q/support/literals.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr auto sps = 48000.0f;

   template <typename F>
   std::vector<float> cycle(F&& f, q::frequency freq)
   {
      q::phase_iterator pi{freq, sps};
      auto n = int(sps / as_double(freq));
      std::vector<float> out;
      out.reserve(n);
      for (int i = 0; i != n; ++i, ++pi)
         out.push_back(f(pi));
      return out;
   }

   float mean(std::vector<float> const& y)
   {
      double sum = 0;
      for (auto v : y)
         sum += v;
      return float(sum / y.size());
   }

   float where_peak(std::vector<float> const& y)
   {
      auto at = std::max_element(y.begin(), y.end()) - y.begin();
      return float(at) / y.size();
   }
}

TEST_CASE("triangle_osc: the symmetric one is the classic triangle")
{
   // The plain triangle: trough at phase 0 for the basic one, and the band
   // limited one shifted a quarter cycle, so it leaves 0 going up.
   auto basic = cycle(q::basic_triangle, 100_Hz);
   CHECK(basic.front() == Approx(-1.0f).margin(0.01f));
   CHECK(where_peak(basic) == Approx(0.5f).margin(0.01f));

   auto bl = cycle(q::triangle, 100_Hz);
   CHECK(bl.front() == Approx(0.0f).margin(0.01f));
   CHECK(where_peak(bl) == Approx(0.25f).margin(0.01f));
}

TEST_CASE("triangle_osc: symmetry moves the turn")
{
   // The rise takes `symmetry` of the cycle. The basic one turns there, and
   // the band limited one half a rise earlier, since it is shifted.
   for (float s : {0.2f, 0.35f, 0.5f, 0.7f, 0.9f})
   {
      q::basic_triangle_osc basic{s};
      q::triangle_osc bl{s};

      CHECK(where_peak(cycle(basic, 100_Hz)) == Approx(s).margin(0.02f));
      CHECK(where_peak(cycle(bl, 100_Hz)) == Approx(s / 2).margin(0.02f));
   }
}

TEST_CASE("triangle_osc: skewed, it still spans the same range")
{
   for (float s : {0.1f, 0.5f, 0.9f})
   {
      q::basic_triangle_osc osc{s};
      auto y = cycle(osc, 100_Hz);

      CHECK(*std::max_element(y.begin(), y.end())
         == Approx(1.0f).margin(0.02f));
      CHECK(*std::min_element(y.begin(), y.end())
         == Approx(-1.0f).margin(0.02f));
      CHECK(mean(y) == Approx(0.0f).margin(0.02f));
   }
}

TEST_CASE("triangle_osc: a skew brings up the even harmonics")
{
   // A symmetric triangle has odd harmonics only. Skewing it walks the wave
   // toward a sawtooth, which has both.
   auto harmonic = [](std::vector<float> const& y, int k)
   {
      double re = 0, im = 0;
      for (std::size_t i = 0; i != y.size(); ++i)
      {
         auto p = 2.0 * 3.14159265358979 * k * i / y.size();
         re += y[i] * std::cos(p);
         im += y[i] * std::sin(p);
      }
      return float(2.0 * std::hypot(re, im) / y.size());
   };

   auto even_odd = [&harmonic](float s)
   {
      q::basic_triangle_osc osc{s};
      auto y = cycle(osc, 100_Hz);
      return harmonic(y, 2) / harmonic(y, 3);
   };

   CHECK(even_odd(0.5f) < 0.05f);             // symmetric: no second
   CHECK(even_odd(0.25f) > 0.5f);             // skewed: plenty
}

TEST_CASE("triangle_osc: the symmetry control is bounded")
{
   q::basic_triangle_osc osc;

   osc.symmetry(-1.0f);
   CHECK(osc.symmetry() > 0.0f);
   osc.symmetry(2.0f);
   CHECK(osc.symmetry() < 1.0f);
}
