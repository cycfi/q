/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman, Nikos Parastatidis. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/highpass.hpp>
#include <q/support/literals.hpp>

#include <algorithm>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr auto sps = 48000.0f;
}

TEST_CASE("one_pole_highpass: stable for every cutoff up to Nyquist")
{
   for (float f = 10.0f; f <= sps / 2; f *= 1.5f)
   {
      q::one_pole_highpass hp{q::frequency(f), sps};
      CHECK(hp.a > 0.0f);
      CHECK(hp.a < 1.0f);

      // A step never overshoots and ends at the input.
      float peak = 0.0f;
      for (int i = 0; i != 100000; ++i)
         peak = std::max(peak, hp(1.0f));
      CHECK(peak <= 1.0f);
      CHECK(hp() == Approx(1.0f).margin(1e-4));
   }
}
