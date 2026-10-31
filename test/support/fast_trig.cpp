/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/base.hpp>

#include <cmath>

namespace q = cycfi::q;

// The error bounds below are what these measure on the float inputs, with
// a little headroom: about 2e-7 for fast_sincos at any angle up to 100
// rad, and 1.2e-5 for fast_atan2 (Abramowitz and Stegun's 1e-5 plus float
// rounding).

TEST_CASE("principal_arg wraps to [-pi, pi]", "[fast_trig]")
{
   for (int i = 0; i <= 400000; ++i)
   {
      float x = -1000.0f + 2000.0f * i / 400000;
      float r = q::principal_arg(x);
      REQUIRE(r >= -float(q::pi));
      REQUIRE(r <= float(q::pi));

      // The same angle: sin and cos agree with the unwrapped one.
      REQUIRE(std::abs(std::sin(double(r)) - std::sin(double(x))) < 1e-4);
      REQUIRE(std::abs(std::cos(double(r)) - std::cos(double(x))) < 1e-4);
   }
   REQUIRE(q::principal_arg(0.0f) == 0.0f);
}

TEST_CASE("fast_sincos matches sin and cos", "[fast_trig]")
{
   double es = 0, ec = 0;
   for (int i = 0; i <= 2000000; ++i)
   {
      float x = float(-100.0 + 200.0 * i / 2000000);
      auto [s, c] = q::fast_sincos(x);
      es = std::max(es, std::abs(s - std::sin(double(x))));
      ec = std::max(ec, std::abs(c - std::cos(double(x))));
   }
   REQUIRE(es < 3e-7);
   REQUIRE(ec < 3e-7);
}

TEST_CASE("fast_atan2 matches atan2 in every octant", "[fast_trig]")
{
   double err = 0;
   for (int i = 0; i <= 1000; ++i)
   {
      for (int j = 0; j <= 1000; ++j)
      {
         float y = -2.0f + 4.0f * i / 1000;
         float x = -2.0f + 4.0f * j / 1000;
         if (x == 0 && y == 0)
            continue;
         double ref = std::atan2(double(y), double(x));
         err = std::max(err, std::abs(q::fast_atan2(y, x) - ref));
      }
   }
   REQUIRE(err < 1.2e-5);

   SECTION("the axes and the origin")
   {
      REQUIRE(q::fast_atan2(0.0f, 0.0f) == 0.0f);
      REQUIRE(q::fast_atan2(0.0f, 1.0f) == 0.0f);
      REQUIRE(std::abs(q::fast_atan2(1.0f, 0.0f) - q::pi/2) < 1.2e-5);
      REQUIRE(std::abs(q::fast_atan2(-1.0f, 0.0f) + q::pi/2) < 1.2e-5);
      REQUIRE(std::abs(q::fast_atan2(0.0f, -1.0f) - q::pi) < 1.2e-5);
   }
}
