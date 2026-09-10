/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Changing an envelope's rates while it is running. A synth's panel moves
// these under the player's hand, and a segment that is sounding must carry
// on from the level it is at: a jump back to where the segment started is
// heard as a click.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/support/literals.hpp>
#include <q/synth/envelope_gen.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr auto sps = 48000.0f;

   q::adsr_envelope_gen::config config()
   {
      return
      {
         50_ms       // attack rate
       , 200_ms      // decay rate
       , -12_dB      // sustain level
       , 5_s         // sustain rate
       , 1_s         // release rate
      };
   }

   // Run the envelope for a while, keeping what it produced.
   std::vector<float> run(q::adsr_envelope_gen& env, std::size_t n)
   {
      std::vector<float> out;
      out.reserve(n);
      for (std::size_t i = 0; i != n; ++i)
         out.push_back(env());
      return out;
   }

   // The largest step between one sample and the next: an envelope moves
   // smoothly, so a big step is a discontinuity.
   float largest_step(std::vector<float> const& v)
   {
      float worst = 0.0f;
      for (std::size_t i = 1; i != v.size(); ++i)
         worst = std::max(worst, std::abs(v[i] - v[i-1]));
      return worst;
   }
}

TEST_CASE("A rate changed mid-release does not jump the level")
{
   auto env = q::adsr_envelope_gen{config(), sps};
   env.attack();
   run(env, std::size_t(sps * 0.5f));     // through attack and decay
   env.release();

   auto const before = run(env, std::size_t(sps * 0.1f));
   auto const level = before.back();

   // The player pulls the release down while the note is ringing.
   env.release_rate(200_ms, sps);

   auto const after = run(env, std::size_t(sps * 0.1f));

   // It carries on from where it was, downward, without a step.
   CHECK(after.front() <= level);
   CHECK(after.front() == Approx(level).margin(0.01));
   CHECK(largest_step(after) < 0.01f);
}

TEST_CASE("A release shortened while it runs still finishes")
{
   auto env = q::adsr_envelope_gen{config(), sps};
   env.attack();
   run(env, std::size_t(sps * 0.5f));
   env.release();
   run(env, std::size_t(sps * 0.1f));

   env.release_rate(100_ms, sps);

   // A tenth of a second of release, plus room: it must reach idle.
   for (int i = 0; i != int(sps * 0.5f) && !env.in_idle_phase(); ++i)
      env();
   CHECK(env.in_idle_phase());
}

TEST_CASE("A release shortened while it runs tapers, it does not cut off")
{
   // Keeping how far through the segment it is, as a fraction of the new
   // width, is what stops a shortened release ending on the spot at
   // whatever level it had reached.
   auto env = q::adsr_envelope_gen{config(), sps};
   env.attack();
   run(env, std::size_t(sps * 0.5f));
   env.release();
   run(env, std::size_t(sps * 0.1f));

   env.release_rate(100_ms, sps);

   std::vector<float> tail;
   for (int i = 0; i != int(sps * 0.5f) && !env.in_idle_phase(); ++i)
      tail.push_back(env());

   REQUIRE(env.in_idle_phase());
   REQUIRE(tail.size() > std::size_t(sps * 0.01f));   // not on the spot
   CHECK(tail.back() < 0.05f);                        // and it got there
   CHECK(largest_step(tail) < 0.01f);
}

TEST_CASE("A rate changed mid-attack does not jump the level")
{
   auto env = q::adsr_envelope_gen{config(), sps};
   env.attack();

   auto const before = run(env, std::size_t(sps * 0.01f));
   auto const level = before.back();

   env.attack_rate(200_ms, sps);

   auto const after = run(env, std::size_t(sps * 0.01f));
   CHECK(after.front() >= level);
   CHECK(after.front() == Approx(level).margin(0.01));
   CHECK(largest_step(after) < 0.01f);
}

TEST_CASE("A rate pushed again unchanged leaves the envelope alone")
{
   // A plugin pushes its settings every block. Pushing the same value
   // must not restart the segment, or a release would never finish.
   auto env = q::adsr_envelope_gen{config(), sps};
   env.attack();
   run(env, std::size_t(sps * 0.5f));
   env.release();

   for (int i = 0; i != int(sps * 2.0f) && !env.in_idle_phase(); ++i)
   {
      env.release_rate(1_s, sps);
      env();
   }
   CHECK(env.in_idle_phase());
}

TEST_CASE("A note started after a rate change runs at the new rate")
{
   // Reconfiguring must still take effect: a segment entered afresh uses
   // the rate it was last given.
   auto slow = q::adsr_envelope_gen{config(), sps};
   auto fast = q::adsr_envelope_gen{config(), sps};
   fast.attack_rate(10_ms, sps);

   slow.attack();
   fast.attack();

   auto const a = run(slow, std::size_t(sps * 0.01f));
   auto const b = run(fast, std::size_t(sps * 0.01f));
   CHECK(b.back() > a.back());
}
