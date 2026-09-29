/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The recursive moving averages: exp_moving_average,
// rt_exp_moving_average and moving_average2.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/moving_average.hpp>
#include <q/support/literals.hpp>

namespace q = cycfi::q;
using namespace q::literals;

TEST_CASE("exp_moving_average: b is 2 / (n + 1)")
{
   using ema = q::exp_moving_average<15>;
   CHECK(ema::b == Approx(2.0f / 16));
   CHECK(ema::b_ == Approx(1.0f - 2.0f / 16));

   ema f;
   CHECK(f(1.0f) == Approx(ema::b));         // from rest
}

TEST_CASE("exp_moving_average: unity gain at DC")
{
   q::exp_moving_average<32> f;
   for (int i = 0; i != 2000; ++i)
      f(0.5f);
   CHECK(f() == Approx(0.5f).margin(1e-5));
}

TEST_CASE("exp_moving_average: construct and assign the state")
{
   q::exp_moving_average<4> f{0.75f};
   CHECK(f() == 0.75f);
   f = 0.25f;
   CHECK(f() == 0.25f);
}

TEST_CASE("rt_exp_moving_average: matches the compile time form")
{
   q::exp_moving_average<10> a;
   q::rt_exp_moving_average b{10};

   for (int i = 0; i != 100; ++i)
   {
      auto s = float(i % 7) - 3.0f;
      CHECK(b(s) == Approx(a(s)).margin(1e-6));
   }
}

TEST_CASE("rt_exp_moving_average: a duration is a span in samples")
{
   // 1 ms at 48 kHz is 48 samples.
   q::rt_exp_moving_average f{1_ms, 48000.0f};
   CHECK(f.b == Approx(2.0f / 49));
}

TEST_CASE("rt_exp_moving_average: a new width keeps unity gain at DC")
{
   q::rt_exp_moving_average f{10};
   f.width(100);
   CHECK(f.b == Approx(2.0f / 101));
   CHECK(f.b_ == Approx(1.0f - f.b));

   for (int i = 0; i != 5000; ++i)
      f(1.0f);
   CHECK(f() == Approx(1.0f).margin(1e-4));
}

TEST_CASE("moving_average2: the mean of this sample and the last")
{
   q::moving_average2 f;
   CHECK(f(2.0f) == 1.0f);                   // (2 + 0) / 2
   CHECK(f(4.0f) == 3.0f);
   CHECK(f(4.0f) == 4.0f);
   CHECK(f() == 4.0f);

   // A Nyquist-rate alternation is cancelled.
   for (int i = 0; i != 8; ++i)
      f((i & 1) ? 1.0f : -1.0f);
   CHECK(f() == 0.0f);
}
