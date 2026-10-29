/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman, Nikos Parastatidis. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/lowpass.hpp>
#include <q/fx/smoothing.hpp>
#include <q/support/literals.hpp>

#include <algorithm>
#include <cmath>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr auto sps = 48000.0f;
   constexpr int max_samples = 5 * 48000;   // 5 seconds is plenty to settle
}

TEST_CASE("snap: snaps to target inside the delta")
{
   q::snap sn{0.01f};

   CHECK(sn(0.995f, 1.0f) == 1.0f);
   CHECK(sn.settled());

   q::snap exact{0.01f};
   CHECK(exact(1.0f, 1.0f) == 1.0f);
   CHECK(exact.settled());
}

TEST_CASE("snap: passes the value through outside the delta")
{
   q::snap sn{0.01f};

   CHECK(sn(0.5f, 1.0f) == 0.5f);
   CHECK_FALSE(sn.settled());

   // Still moving, still not settled.
   CHECK(sn(0.6f, 1.0f) == 0.6f);
   CHECK_FALSE(sn.settled());
}

TEST_CASE("snap: snaps when the input stalls short of the target")
{
   // Float precision can leave the smoother stuck just short of the target.
   q::snap sn{1e-9f};

   CHECK(sn(0.9f, 1.0f) == 0.9f);
   CHECK_FALSE(sn.settled());

   CHECK(sn(0.9f, 1.0f) == 1.0f);   // same value twice -> stalled
   CHECK(sn.settled());
}

TEST_CASE("snap: unsettles when the target moves away")
{
   q::snap sn{0.01f};

   sn(1.0f, 1.0f);
   REQUIRE(sn.settled());

   CHECK(sn(0.5f, 0.0f) == 0.5f);
   CHECK_FALSE(sn.settled());
}

TEST_CASE("smoothed: converges to the target and settles, for every time constant")
{
   for (auto time : {1_ms, 10_ms, 100_ms})
   {
      q::smoothed<q::one_pole_lowpass> sm{time, sps, 1e-4f};

      int i = 0;
      float y = 0.0f;
      for (; i != max_samples && !sm.settled(); ++i)
         y = sm(1.0f);

      CHECK(sm.settled());
      CHECK(i < max_samples);
      CHECK(y == 1.0f);          // snapped exactly, not just close
      CHECK(sm() != 0.0f);
   }
}

TEST_CASE("smoothed: a step never overshoots and rises monotonically")
{
   q::smoothed<q::one_pole_lowpass> sm{10_ms, sps, 1e-4f};

   float prev = 0.0f;
   for (int i = 0; i != 100000; ++i)
   {
      float y = sm(1.0f);
      CHECK(y >= prev);
      CHECK(y <= 1.0f);
      prev = y;
   }
   CHECK(prev == 1.0f);
}

TEST_CASE("smoothed: reaches ~63% of a step after one time constant")
{
   q::smoothed<q::one_pole_lowpass> sm{10_ms, sps, 1e-6f};

   float y = 0.0f;
   int n = int(0.010f * sps);
   for (int i = 0; i != n; ++i)
      y = sm(1.0f);

   CHECK(y == Approx(1.0f - std::exp(-1.0f)).margin(0.03));
}

TEST_CASE("smoothed: tracks a new target after settling")
{
   q::smoothed<q::one_pole_lowpass> sm{10_ms, sps, 1e-4f};

   for (int i = 0; i != max_samples && !sm.settled(); ++i)
      sm(1.0f);
   REQUIRE(sm.settled());

   float y = sm(0.0f);
   CHECK_FALSE(sm.settled());
   CHECK(y < 1.0f);
   CHECK(y > 0.0f);

   for (int i = 0; i != max_samples && !sm.settled(); ++i)
      y = sm(0.0f);

   CHECK(sm.settled());
   CHECK(y == 0.0f);
}

TEST_CASE("smoothed: reading the value does not advance the smoother")
{
   q::smoothed<q::one_pole_lowpass> sm{10_ms, sps, 1e-4f};

   sm(1.0f);
   float a = sm();
   float b = sm();
   CHECK(a == b);
   CHECK(a > 0.0f);
}

TEST_CASE("smoothed: assignment jumps straight to the value")
{
   q::smoothed<q::one_pole_lowpass> sm{10_ms, sps, 1e-4f};

   sm = 0.5f;
   CHECK(sm() == Approx(0.5f));

   // Holding the same target after the jump stays put.
   CHECK(sm(0.5f) == Approx(0.5f));
   CHECK(sm.settled());
}

TEST_CASE("smoothed: using dynamic smoother works as well as one-pole")
{
   for (auto time : {1_ms, 10_ms, 100_ms})
   {
      q::smoothed<q::dynamic_smoother> sm{time, sps, 1e-4f};

      int i = 0;
      float y = 0.0f;
      for (; i != max_samples && !sm.settled(); ++i)
         y = sm(1.0f);

      CHECK(sm.settled());
      CHECK(i < max_samples);
      CHECK(y == 1.0f);          // snapped exactly, not just close
      CHECK(sm() != 0.0f);
   }
}