/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/clip.hpp>
#include <cmath>

namespace q = cycfi::q;

TEST_CASE("hard_clip")
{
   q::hard_clip clip{};                    // default max 1.0
   CHECK(clip(0.5f) == Approx(0.5f));      // pass-through within range
   CHECK(clip(2.0f) == Approx(1.0f));      // clamped
   CHECK(clip(-2.0f) == Approx(-1.0f));

   q::hard_clip clip_q{0.25f};             // custom max
   CHECK(clip_q(0.5f) == Approx(0.25f));
   CHECK(clip_q(-0.5f) == Approx(-0.25f));
   CHECK(clip_q(0.1f) == Approx(0.1f));
}

TEST_CASE("cubic_clip")
{
   q::cubic_clip sc{};
   CHECK(sc(0.0f) == Approx(0.0f));
   CHECK(sc(1.0f) == Approx(1.0f));        // 1.5 - 0.5 = 1
   CHECK(sc(-1.0f) == Approx(-1.0f));
   CHECK(sc(2.0f) == Approx(1.0f));        // pre-clipped to 1, then cubic -> 1
   for (float x = -3.0f; x <= 3.0f; x += 0.1f)
   {
      CHECK(sc(x) <= 1.0f);
      CHECK(sc(x) >= -1.0f);
   }
}

TEST_CASE("tanh_clip")
{
   q::tanh_clip sc{};                       // exp-based fast_tanh (approximate)
   CHECK(sc(0.0f) == Approx(0.0f).margin(0.01f));

   // bounded, monotonic non-decreasing, approximates tanh
   float prev = sc(-6.0f);
   for (float x = -6.0f; x <= 6.0f; x += 0.05f)
   {
      float y = sc(x);
      CHECK(y <= 1.001f);
      CHECK(y >= -1.001f);
      CHECK(y >= prev - 1e-3f);
      prev = y;
   }
   CHECK(sc(-1.0f) == Approx(-sc(1.0f)).margin(0.02f));      // ~odd
   for (float x = -3.0f; x <= 3.0f; x += 0.5f)
      CHECK(sc(x) == Approx(std::tanh(x)).margin(0.05f));    // ~tanh
}
