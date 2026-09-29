/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/sample_hold.hpp>
#include <q/fx/delta_gate.hpp>
#include <q/support/literals.hpp>

namespace q = cycfi::q;
using namespace q::literals;

TEST_CASE("sample_hold: lookback runs d to 1.5d")
{
   // d = 8 samples: a hold every 4. Feed a ramp 1, 2, 3, ...
   q::sample_hold sh{8u / 2};
   int first = 0;
   for (int i = 1; i <= 40; ++i)
   {
      float then = sh(float(i));
      if (then > 0.0f && first == 0)
         first = i;
      if (then > 0.0f)
      {
         // The held value is between 8 and 12 samples old.
         CHECK(i - int(then) >= 8);
         CHECK(i - int(then) <= 12);
      }
   }
   CHECK(first > 0);
   CHECK(sh() > 0.0f);
}

TEST_CASE("delta_gate: fires on a rise against the past")
{
   // 1 kHz, d = 8 ms: a hold every 4 samples.
   q::delta_gate g{6_dB, 8_ms, 1000.0f};

   // Silence, then a level: a rise against nothing.
   for (int i = 0; i != 20; ++i)
      CHECK(!g(0.0f));
   CHECK(g(1.0f));
   for (int i = 0; i != 20; ++i)
      g(1.0f);
   CHECK(!g());

   // A 12 dB step above the held level fires once the past is the level.
   bool fired = false;
   for (int i = 0; i != 20; ++i)
      fired |= g(4.0f);
   CHECK(fired);

   // Holding there, the reference catches up and the gate shuts.
   for (int i = 0; i != 20; ++i)
      g(4.0f);
   CHECK(!g());

   // A fall does not fire the unipolar gate.
   fired = false;
   for (int i = 0; i != 20; ++i)
      fired |= g(1.0f);
   CHECK(!fired);
}

TEST_CASE("delta_gate_bipolar: fires either way, shut while invalid")
{
   q::delta_gate_bipolar g{1.0594631f, 8_ms, 1000.0f};      // a semitone

   for (int i = 0; i != 20; ++i)
      CHECK(!g(0.0f));
   for (int i = 0; i != 20; ++i)
      g(100.0f);

   bool fired = false;
   for (int i = 0; i != 20; ++i)
      fired |= g(110.0f);      // up a whole tone
   CHECK(fired);

   for (int i = 0; i != 20; ++i)
      g(110.0f);
   fired = false;
   for (int i = 0; i != 20; ++i)
      fired |= g(100.0f);      // and back down
   CHECK(fired);

   // A change of less than the ratio, or to zero, does not fire.
   for (int i = 0; i != 20; ++i)
      g(100.0f);
   fired = false;
   for (int i = 0; i != 20; ++i)
      fired |= g(103.0f);
   CHECK(!fired);
   fired = false;
   for (int i = 0; i != 20; ++i)
      fired |= g(0.0f);
   CHECK(!fired);
}

TEST_CASE("sample_hold: a duration is a hold every d / 2")
{
   // 8 ms at 1 kHz is a hold every 4 samples, as sample_hold{4u}.
   q::sample_hold a{8_ms, 1000.0f};
   q::sample_hold b{4u};
   for (int i = 1; i <= 40; ++i)
      CHECK(a(float(i)) == b(float(i)));
}

TEST_CASE("sample_hold: at least one sample per hold")
{
   // A hold every sample: the reference is three samples back.
   q::sample_hold sh{0u};
   for (int i = 1; i <= 10; ++i)
   {
      float then = sh(float(i));
      if (i > 3)
         CHECK(then == float(i - 3));
   }
}

TEST_CASE("delta_gate: a plain ratio is the decibel ratio")
{
   q::delta_gate a{6_dB, 8_ms, 1000.0f};
   q::delta_gate b{q::lin_float(6_dB), 8_ms, 1000.0f};
   for (int i = 0; i != 60; ++i)
   {
      float s = (i < 30) ? 1.0f : 3.0f;
      CHECK(a(s) == b(s));
      CHECK(a.then() == b.then());
   }
}

TEST_CASE("delta_gate: then() is the reference the next sample meets")
{
   q::delta_gate g{2.0f, 8_ms, 1000.0f};
   for (int i = 0; i != 40; ++i)
      g(1.0f);
   CHECK(g.then() == 1.0f);

   // Just under twice the reference: shut. Just over: open.
   CHECK(!g(1.99f));
   CHECK(g(2.01f));
}
