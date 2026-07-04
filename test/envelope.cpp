/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/synth/envelope_gen.hpp>
#include <algorithm>
#include <cmath>
#include <random>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr float sps = 48000.0f;

   q::adsr_envelope_gen::config make_config()
   {
      return q::adsr_envelope_gen::config
      {
         10_ms     // attack
       , 50_ms     // decay
       , -12_dB    // sustain level (~0.251 linear)
       , 5_s       // sustain
       , 50_ms     // release
      };
   }

   std::size_t samples(q::duration d) { return std::size_t(q::as_float(d) * sps); }

   // Run n samples, return the last value.
   float run(q::adsr_envelope_gen& e, std::size_t n)
   {
      float y = 0;
      for (std::size_t i = 0; i < n; ++i)
         y = e();
      return y;
   }

   // Run n samples, return the peak value.
   float run_peak(q::adsr_envelope_gen& e, std::size_t n)
   {
      float pk = 0;
      for (std::size_t i = 0; i < n; ++i)
         pk = std::max(pk, e());
      return pk;
   }
}

TEST_CASE("Idle output is zero")
{
   q::adsr_envelope_gen e{make_config(), sps};
   CHECK(e.in_idle_phase());
   CHECK(run_peak(e, 1000) == 0.0f);
}

TEST_CASE("Attack from idle reaches the peak")
{
   q::adsr_envelope_gen e{make_config(), sps};
   e.attack();
   CHECK(e.in_attack_phase());
   CHECK(run_peak(e, samples(10_ms) + 16) == Approx(1.0).margin(0.05));
}

TEST_CASE("Decays to the sustain level")
{
   q::adsr_envelope_gen e{make_config(), sps};
   e.attack();
   run(e, samples(10_ms) + samples(50_ms) + 32);   // through attack + decay
   CHECK(e.current() == Approx(q::lin_float(-12_dB)).margin(0.03));
}

TEST_CASE("Release falls to zero")
{
   q::adsr_envelope_gen e{make_config(), sps};
   e.attack();
   run(e, samples(10_ms) + samples(50_ms) + 32);
   e.release();
   CHECK(run(e, samples(50_ms) + 256) < 0.02f);
}

// Regression: prior to the fix, attack() was gated on in_idle_phase(), so a
// note arriving before the previous release finished was silently ignored and
// the level just kept decaying (the cause of fluctuating arpeggios). attack()
// must retrigger from any phase.
TEST_CASE("Attack retriggers during release")
{
   q::adsr_envelope_gen e{make_config(), sps};
   e.attack();
   run(e, samples(10_ms) + 16);                 // reach the peak
   e.release();
   run(e, samples(20_ms));                      // partway down the release
   REQUIRE(e.current() < 0.9f);                 // we are below the peak
   REQUIRE(e.in_release_phase());

   e.attack();                                  // retrigger
   CHECK(e.in_attack_phase());
   CHECK(run_peak(e, samples(10_ms) + 16) == Approx(1.0).margin(0.05));
}

TEST_CASE("Attack retriggers during sustain")
{
   q::adsr_envelope_gen e{make_config(), sps};
   e.attack();
   run(e, samples(10_ms) + samples(50_ms) + 32);   // into sustain
   REQUIRE(e.current() < 0.5f);

   e.attack();                                      // retrigger
   CHECK(run_peak(e, samples(10_ms) + 16) == Approx(1.0).margin(0.05));
}

TEST_CASE("Retrigger ramps from the current level (click-free)")
{
   q::adsr_envelope_gen e{make_config(), sps};
   e.attack();
   run(e, samples(30_ms));                  // somewhere in decay
   float before = e.current();
   REQUIRE(before > 0.3f);
   REQUIRE(before < 1.0f);

   e.attack();                              // retrigger
   float first = e();
   CHECK(first == Approx(before).margin(0.15));   // no jump to zero
   CHECK(run_peak(e, samples(8_ms)) > before);    // and it rises (retriggered)
}

// ---------------------------------------------------------------------------
// Stress tests: hammer attack()/release() in every phase combination and assert
// the invariants that must always hold. These guard the whole state machine,
// not just attack(), against the kind of latent breakage the 2023 rewrite left.
// ---------------------------------------------------------------------------

TEST_CASE("Stress: invariants hold under chaotic triggering", "[.][stress]")
{
   q::adsr_envelope_gen e{make_config(), sps};
   std::mt19937 rng{0xC0FFEE};
   std::uniform_int_distribution<int> action{0, 3};       // 0=attack 1=release else run
   std::uniform_int_distribution<int> dur{1, 1500};

   // Accumulate violations into a flag (plain checks, not per-sample Catch
   // assertions) so the millions of samples stay fast; report once at the end.
   float prev = 0.0f;
   bool finite = true, bounded = true, continuous = true;
   float worst_delta = 0.0f;
   for (int step = 0; step < 30000; ++step)
   {
      switch (action(rng))
      {
         case 0: e.attack();  break;
         case 1: e.release(); break;
         default: break;                                   // just let it evolve
      }
      int n = dur(rng);
      for (int i = 0; i < n; ++i)
      {
         float y = e();
         if (!std::isfinite(y)) finite = false;
         if (y < -1e-3f || y > 1.0f + 1e-3f) bounded = false;
         float d = std::abs(y - prev);
         if (d > worst_delta) worst_delta = d;
         if (d > 0.15f) continuous = false;
         prev = y;
      }
   }
   CHECK(finite);                                          // never NaN/inf
   CHECK(bounded);                                         // stays within [0, 1]
   CHECK(continuous);                                      // no clicks
   INFO("worst per-sample delta: " << worst_delta);
   CHECK(worst_delta <= 0.15f);
}

TEST_CASE("Stress: attack from any phase reaches the peak", "[.][stress]")
{
   q::adsr_envelope_gen e{make_config(), sps};
   std::mt19937 rng{42};
   std::uniform_int_distribution<int> dur{0, int(samples(300_ms))};

   for (int t = 0; t < 600; ++t)
   {
      // Drive into a random phase, with a random pending release.
      e.attack();
      run(e, dur(rng));
      if (t & 1) e.release();
      run(e, dur(rng));

      e.attack();                                          // retrigger from wherever
      CHECK(run_peak(e, samples(10_ms) + 16) == Approx(1.0).margin(0.05));
   }
}

TEST_CASE("Stress: release from any phase reaches idle and zero", "[.][stress]")
{
   q::adsr_envelope_gen e{make_config(), sps};
   std::mt19937 rng{7};
   std::uniform_int_distribution<int> dur{0, int(samples(300_ms))};

   for (int t = 0; t < 600; ++t)
   {
      e.attack();
      run(e, dur(rng));                                    // stop somewhere mid-note
      e.release();
      float end = run(e, samples(50_ms) + 256);            // release_rate + slack
      CHECK(end < 0.02f);
      CHECK(e.in_idle_phase());
   }
}
