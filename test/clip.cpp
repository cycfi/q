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
   q::clip clip{};                         // default max 1.0
   CHECK(clip(0.5f) == Approx(0.5f));      // pass-through within range
   CHECK(clip(2.0f) == Approx(1.0f));      // clamped
   CHECK(clip(-2.0f) == Approx(-1.0f));

   q::clip clip_q{0.25f};                  // custom max
   CHECK(clip_q(0.5f) == Approx(0.25f));
   CHECK(clip_q(-0.5f) == Approx(-0.25f));
   CHECK(clip_q(0.1f) == Approx(0.1f));
}

TEST_CASE("soft_clip_cubic")
{
   q::soft_clip sc{};
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

TEST_CASE("soft_clip2_rational_tanh")
{
   q::soft_clip2 sc{};

   // passes through 0 with unit slope
   CHECK(sc(0.0f) == Approx(0.0f));
   CHECK((sc(0.001f) - sc(-0.001f)) / 0.002f == Approx(1.0f).margin(1e-3f));

   // bounded to [-1, +1] across a wide range, including past the +/-3 knee
   for (float x = -20.0f; x <= 20.0f; x += 0.1f)
   {
      CHECK(sc(x) <= 1.0f);
      CHECK(sc(x) >= -1.0f);
   }

   // odd symmetry
   for (float x = 0.0f; x <= 5.0f; x += 0.25f)
      CHECK(sc(-x) == Approx(-sc(x)));

   // monotonic non-decreasing
   float prev = sc(-6.0f);
   for (float x = -6.0f; x <= 6.0f; x += 0.05f)
   {
      float y = sc(x);
      CHECK(y >= prev - 1e-6f);
      prev = y;
   }

   // knee endpoints map exactly to +/-1; saturates (clamped) beyond
   CHECK(sc(3.0f) == Approx(1.0f));
   CHECK(sc(-3.0f) == Approx(-1.0f));
   CHECK(sc(100.0f) == Approx(1.0f));

   // approximates std::tanh within the valid range
   for (float x = -3.0f; x <= 3.0f; x += 0.5f)
      CHECK(sc(x) == Approx(std::tanh(x)).margin(0.03f));
}
