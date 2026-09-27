/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/synth/sample_hold_osc.hpp>
#include <q/synth/gen/noise_gen.hpp>
#include <cmath>
#include <set>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr float sps = 48000.0f;
}

TEST_CASE("It samples the signal at each wrap and holds it")
{
   q::sample_hold_osc sh;
   q::phase_iterator pi{50_Hz, sps};               // ~960 samples a cycle
   float held = sh(pi++, 0.0f);
   int wraps = 0;
   for (int i = 1; i != 960 * 10; ++i)
   {
      auto s = std::sin(i * 0.001f);
      bool wrap = pi.first();
      auto y = sh(pi++, s);
      if (wrap)
      {
         held = s;
         ++wraps;
      }
      REQUIRE(y == held);
   }
   CHECK(wraps >= 9);
}

TEST_CASE("A reset of the phase takes a new sample")
{
   q::sample_hold_osc sh;
   q::phase_iterator pi{10_Hz, sps};               // 4800 samples a cycle
   for (int i = 0; i != 6000; ++i)                 // past one wrap
      sh(pi++, 1.0f);
   CHECK(sh(pi, 2.0f) == 1.0f);
   pi._phase = q::phase{};
   CHECK(sh(pi, 3.0f) == 3.0f);
}

TEST_CASE("Fed white noise: a new value per cycle, spanning -1 to 1")
{
   q::sample_hold_osc sh;
   q::phase_iterator pi{100_Hz, sps};
   std::set<float> values;
   for (int i = 0; i != 480 * 50; ++i)
   {
      auto y = sh(pi++, q::white_noise());
      REQUIRE(std::abs(y) <= 1.0f);
      values.insert(y);
   }
   CHECK(values.size() > 45);
   CHECK(*values.begin() < -0.5f);
   CHECK(*values.rbegin() > 0.5f);
}
