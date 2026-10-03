/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/base.hpp>

#include <cmath>

namespace q = cycfi::q;

TEST_CASE("fast_tanh: finite over the whole real line")
{
   // The exp under it overflows for a large negative argument, which used to
   // come back as NaN. A driven saturator reaches that range easily.
   for (float x = -1000.0f; x <= 1000.0f; x += 0.5f)
   {
      REQUIRE(std::isfinite(q::fast_tanh(x)));
      REQUIRE(std::isfinite(q::faster_tanh(x)));
      REQUIRE(std::abs(q::fast_tanh(x)) <= 1.0f);
   }
}

TEST_CASE("fast_tanh: saturates to +/-1")
{
   CHECK(q::fast_tanh(50.0f) == Approx(1.0f));
   CHECK(q::fast_tanh(-50.0f) == Approx(-1.0f));
   CHECK(q::fast_tanh(1e6f) == Approx(1.0f));
   CHECK(q::fast_tanh(-1e6f) == Approx(-1.0f));
}

TEST_CASE("fast_tanh: tracks std::tanh where it matters")
{
   for (float x = -8.0f; x <= 8.0f; x += 0.05f)
      CHECK(q::fast_tanh(x) == Approx(std::tanh(x)).margin(0.002f));
}

TEST_CASE("fast_tanh: odd and monotone")
{
   // Odd to within the approximation's own error, which is about 2e-3; the
   // exp under it is not exactly symmetric.
   for (float x = 0.0f; x <= 12.0f; x += 0.1f)
      CHECK(q::fast_tanh(-x) == Approx(-q::fast_tanh(x)).margin(2e-4f));

   auto prev = q::fast_tanh(-12.0f);
   for (float x = -12.0f; x <= 12.0f; x += 0.01f)
   {
      auto y = q::fast_tanh(x);
      REQUIRE(y >= prev - 1e-6f);
      prev = y;
   }
}
