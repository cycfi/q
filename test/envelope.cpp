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
