/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/support/literals.hpp>
#include <q/synth/gen/envelope_gen.hpp>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 48000.0f;

namespace
{
   auto make_config()
   {
      return q::adsr_envelope_gen::config{
         4_ms,       // attack rate
         250_ms,     // decay rate
         -9_dB,      // sustain level
         30_s,       // sustain rate
         120_ms      // release rate
      };
   }

   // Run an envelope for a while, then release it and report how long the
   // release took and the level it was at when it ended.
   struct release_result
   {
      std::size_t    samples;
      float          last_level;
   };

   release_result run_and_release(q::adsr_envelope_gen& env, double hold)
   {
      env.attack();
      for (std::size_t i = 0; i != std::size_t(hold * sps); ++i)
         env();

      env.release();
      auto n = std::size_t{0};
      auto y = 0.0f;
      while (!env.in_idle_phase() && n != std::size_t(sps))
      {
         y = env();
         ++n;
      }
      return {n, y};
   }
}

TEST_CASE("TEST_envelope_copy_is_independent")
{
   // A voice pool built by copying one prototype is the common idiom, and
   // the segments hold their ramps by pointer, so a copy has to make its
   // own. Sharing them shows up when the voices overlap: one envelope
   // consumes the release ramp, and the next one to be released finds it
   // finished and jumps to idle at full level, which is heard as a click.
   q::adsr_envelope_gen prototype{make_config(), sps};
   std::vector<q::adsr_envelope_gen> pool(4, prototype);

   auto run = [](q::adsr_envelope_gen& env, double seconds)
   {
      for (std::size_t i = 0; i != std::size_t(seconds * sps); ++i)
         env();
   };

   pool[0].attack();
   run(pool[0], 0.24);
   pool[1].attack();                      // the next note, while the first
   run(pool[1], 0.03);                    // is still sounding

   pool[0].release();
   run(pool[0], 0.13);                    // its release finishes
   CHECK(pool[0].in_idle_phase());

   // The second voice is untouched by any of that.
   CHECK(!pool[1].in_idle_phase());
   auto level = pool[1].current();
   CHECK(level > 0.3f);

   pool[1].release();
   auto n = std::size_t{0};
   auto y = level;
   while (!pool[1].in_idle_phase() && n != std::size_t(sps))
   {
      y = pool[1]();
      ++n;
   }
   CHECK(n == std::size_t(0.120 * sps));  // the full release, not a jump
   CHECK(y == Approx(0.0f).margin(1e-4)); // ending at zero, not mid level
}

TEST_CASE("TEST_envelope_copy_does_not_track_the_original")
{
   q::adsr_envelope_gen a{make_config(), sps};
   auto b = a;

   a.attack();
   for (std::size_t i = 0; i != std::size_t(0.1 * sps); ++i)
      a();

   // b has not been touched, so it is still idle and silent.
   CHECK(b.in_idle_phase());
   CHECK(b() == 0.0f);

   // Advancing b must not move a.
   auto a_level = a.current();
   b.attack();
   for (std::size_t i = 0; i != std::size_t(0.05 * sps); ++i)
      b();
   CHECK(a.current() == Approx(a_level));
   CHECK(b.current() > a_level);          // b is 50 ms into its own decay
}

TEST_CASE("TEST_envelope_assignment_is_independent")
{
   q::adsr_envelope_gen a{make_config(), sps};
   q::adsr_envelope_gen b{make_config(), sps};

   a.attack();
   for (std::size_t i = 0; i != std::size_t(0.05 * sps); ++i)
      a();

   b = a;                                 // mid-note, ramps and all
   CHECK(b.current() == Approx(a.current()));

   auto a_before = a.current();
   for (std::size_t i = 0; i != std::size_t(0.05 * sps); ++i)
      b();
   CHECK(a.current() == Approx(a_before));
   CHECK(b.current() < a_before);         // only b moved
}
