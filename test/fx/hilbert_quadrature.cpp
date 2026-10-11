/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/hilbert_quadrature.hpp>
#include <q/support/literals.hpp>
#include <cmath>

namespace q = cycfi::q;
using namespace q::literals;

TEST_CASE("hilbert_quadrature: atan2(q, i) advances by 2 pi f / sps")
{
   constexpr float sps = 48000.0f;
   constexpr int settle = 4000;
   constexpr int n = 8000;

   for (float f : {100.0f, 1000.0f, 5000.0f, 15000.0f})
   {
      q::hilbert_quadrature hq;
      double const expected = 2_pi * f / sps;
      double prev = 0.0;
      for (int k = 0; k < settle + n; ++k)
      {
         auto [i, qd] = hq(std::sin(2_pi * f * k / sps));
         double phase = std::atan2(qd, i);
         if (k > settle)
         {
            // Unwrap the step to (-pi, pi]
            double step = phase - prev;
            if (step > q::pi)
               step -= 2_pi;
            else if (step <= -q::pi)
               step += 2_pi;
            CHECK(step == Approx(expected).epsilon(0.02));
         }
         prev = phase;
      }
   }
}
