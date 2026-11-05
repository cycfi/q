/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/utility/zero_crossing_collector.hpp>
#include <q/support/literals.hpp>

#include <cmath>
#include <numbers>

namespace q = cycfi::q;
using namespace q::literals;

TEST_CASE("zero_crossing_collector: edge delays")
{
   // A sine of period 100.25 samples. A rising edge is taken half the
   // hysteresis above zero, so every crossing reads the same small bias
   // late; past that, successive edges are a period apart, and an edge's
   // delay grows by one a sample, whatever the window does in between.
   double const period = 100.25;
   q::zero_crossing_collector zc{-40_dB, 1024};
   double const bias = std::asin(0.005) / (2 * std::numbers::pi) * period;
   std::size_t checked = 0;
   for (std::size_t t = 0; t != 20000; ++t)
   {
      double phase = 2 * std::numbers::pi * double(t) / period;
      zc(float(std::sin(phase)));
      if (zc.num_edges() < 2 || t % 37 != 0)
         continue;

      for (std::size_t i = 0; i != zc.num_edges(); ++i)
      {
         double crossing = double(t) - zc.delay(i) - bias;
         double k = std::round(crossing / period);
         REQUIRE(crossing == Approx(k * period).margin(0.02));
         if (i > 0)
         {
            REQUIRE(zc.delay(i - 1) - zc.delay(i)
               == Approx(period).margin(0.02));
         }
      }
      ++checked;
   }
   CHECK(checked > 400);
}

TEST_CASE("zero_crossing_collector: a capacity keeps the newest edges")
{
   // A sine of period 100 samples, 16 edges kept: once more than 16 have
   // passed the oldest go, and the newest 16 stay a period apart.
   q::zero_crossing_collector zc{-40_dB, 8192, 16};
   REQUIRE(zc.capacity() == 16);
   for (std::size_t t = 0; t != 4000; ++t)
   {
      zc(float(std::sin(2 * std::numbers::pi * double(t) / 100)));
      REQUIRE(zc.num_edges() <= 16);
   }
   REQUIRE(zc.num_edges() == 16);
   for (std::size_t i = 1; i != 16; ++i)
      REQUIRE(zc.delay(i - 1) - zc.delay(i) == Approx(100.0).margin(0.02));
   REQUIRE(zc.delay(15) < 100);
}

TEST_CASE("zero_crossing_collector: a hysteresis per sample")
{
   // A sine with a ripple on it: at a hysteresis under the ripple the
   // wiggles around each zero cross too, at one above it only the sine
   // does.
   auto signal = [](std::size_t t)
   {
      return float(std::sin(2 * std::numbers::pi * double(t) / 400)
         + 0.2 * std::sin(2 * std::numbers::pi * double(t) / 7));
   };
   q::zero_crossing_collector fine{-60_dB, 1024};
   q::zero_crossing_collector coarse{-60_dB, 1024};
   std::size_t fine_edges = 0, coarse_edges = 0;
   bool fine_was = false, coarse_was = false;
   for (std::size_t t = 0; t != 1000; ++t)
   {
      bool const f = fine(signal(t), 0.01f);
      bool const c = coarse(signal(t), 0.5f);
      fine_edges += f && !fine_was;
      coarse_edges += c && !coarse_was;
      fine_was = f;
      coarse_was = c;
   }
   REQUIRE(fine_edges > 10);
   REQUIRE(coarse_edges == 3);
}
