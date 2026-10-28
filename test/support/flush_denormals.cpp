/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/flush_denormals.hpp>

#include <cmath>

namespace q = cycfi::q;

namespace
{
   // Through volatile, so the compiler cannot fold the arithmetic at
   // compile time, where the thread's mode does not apply.
   volatile float tiny = 1e-30f;
   volatile float small = 1e-15f;

   float product() { return tiny * small; }     // 1e-45: subnormal
}

TEST_CASE("TEST_flush_denormals")
{
   // Without the guard, the product is subnormal.
   REQUIRE(std::fpclassify(product()) == FP_SUBNORMAL);

   {
      // With it, the product flushes to zero.
      q::flush_denormals guard;
      CHECK(product() == 0.0f);

      // Nested, it leaves the outer guard's mode in place.
      {
         q::flush_denormals inner;
         CHECK(product() == 0.0f);
      }
      CHECK(product() == 0.0f);
   }

   // Out of scope, the thread's mode is back.
   CHECK(std::fpclassify(product()) == FP_SUBNORMAL);
}
