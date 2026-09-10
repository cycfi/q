/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// What the envelope does while a key is held. Q's sustain decays, which is
// its own idea; the classic ADSR holds. Which one you get is decided by the
// config you hand it: one with a sustain_rate decays, one without holds.
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

   // A config of the caller's own, with no sustain rate in it.
   struct holding_config
   {
      q::duration    attack_rate    = 20_ms;
      q::duration    decay_rate     = 100_ms;
      q::decibel     sustain_level  = -12_dB;
      q::duration    release_rate   = 200_ms;
   };

   std::vector<float> run(q::adsr_envelope_gen& env, std::size_t n)
   {
      std::vector<float> out;
      out.reserve(n);
      for (std::size_t i = 0; i != n; ++i)
         out.push_back(env());
      return out;
   }

   float largest_step(std::vector<float> const& v)
   {
      float worst = 0.0f;
      for (std::size_t i = 1; i != v.size(); ++i)
         worst = std::max(worst, std::abs(v[i] - v[i-1]));
      return worst;
   }
}

TEST_CASE("A config is anything carrying the four rates the envelope uses")
{
   // The sustain rate is not among them: that is what makes it optional.
   static_assert(q::concepts::ADSRConfig<holding_config>);
   static_assert(q::concepts::ADSRConfig<q::adsr_envelope_gen::config>);

   struct missing_release
   {
      q::duration    attack_rate;
      q::duration    decay_rate;
      q::decibel     sustain_level;
   };
   static_assert(!q::concepts::ADSRConfig<missing_release>);

   struct wrong_types
   {
      float          attack_rate;
      float          decay_rate;
      float          sustain_level;
      float          release_rate;
   };
   static_assert(!q::concepts::ADSRConfig<wrong_types>);

   CHECK(true);      // the assertions above are the test
}

TEST_CASE("A config without a sustain rate holds the sustain level")
{
   auto env = q::adsr_envelope_gen{holding_config{}, sps};
   env.attack();
   run(env, std::size_t(sps * 0.5f));           // through attack and decay

   auto const held = run(env, std::size_t(sps * 4.0f));

   // Four seconds later it is still there, at the level it was asked for.
   CHECK(held.front() == Approx(q::lin_float(-12_dB)).margin(0.01));
   CHECK(held.back() == Approx(held.front()).margin(0.001));
   CHECK(largest_step(held) < 0.001f);
}

TEST_CASE("A held sustain waits however long the key is down")
{
   auto env = q::adsr_envelope_gen{holding_config{}, sps};
   env.attack();
   run(env, std::size_t(sps * 30.0f));          // half a minute
   CHECK(!env.in_idle_phase());
   CHECK(!env.in_release_phase());
}

TEST_CASE("A held sustain still ends when it is released")
{
   auto env = q::adsr_envelope_gen{holding_config{}, sps};
   env.attack();
   run(env, std::size_t(sps * 2.0f));
   env.release();

   for (int i = 0; i != int(sps * 2.0f) && !env.in_idle_phase(); ++i)
      env();
   CHECK(env.in_idle_phase());
}

TEST_CASE("The sustain takes over from the decay without a step")
{
   auto env = q::adsr_envelope_gen{holding_config{}, sps};
   env.attack();

   // Across the whole attack and decay and into the sustain.
   auto const all = run(env, std::size_t(sps * 1.0f));
   CHECK(largest_step(all) < 0.01f);
}

TEST_CASE("A config with a sustain rate decays through the sustain")
{
   // Q's own envelope, unchanged: the sustain is a segment that runs down.
   auto const cfg = q::adsr_envelope_gen::config
   {
      20_ms, 100_ms, -12_dB, 1_s, 200_ms
   };
   auto env = q::adsr_envelope_gen{cfg, sps};
   env.attack();
   run(env, std::size_t(sps * 0.5f));

   auto const a = run(env, std::size_t(sps * 0.2f));
   CHECK(a.back() < a.front());                 // it is going down
}

TEST_CASE("The sustain level can be moved after the envelope is built")
{
   // The decay is what descends to the sustain, so the two carry the same
   // level and both have to move. Setting only the sustain leaves the
   // decay landing where the sustain used to be, and the sustain then
   // cannot hold below that: a floor the panel cannot get under.
   for (double frac : {0.0, 0.1, 0.25, 0.5, 1.0})
   {
      auto env = q::adsr_envelope_gen{holding_config{}, sps};
      env.sustain_level(q::lin_to_db(frac));
      env.attack();

      auto const held = run(env, std::size_t(sps * 1.0f));
      CHECK(held.back() == Approx(frac).margin(0.01));
   }
}

TEST_CASE("The sustain level may be given as a plain fraction")
{
   // The same level either way: the decibel form is a conversion onto
   // the linear one, and a caller holding a fraction skips it.
   auto a = q::adsr_envelope_gen{holding_config{}, sps};
   auto b = q::adsr_envelope_gen{holding_config{}, sps};
   a.sustain_level(q::lin_to_db(0.3));
   b.sustain_level(0.3f);
   a.attack();
   b.attack();

   auto const ya = run(a, std::size_t(sps * 1.0f));
   auto const yb = run(b, std::size_t(sps * 1.0f));
   CHECK(ya.back() == Approx(0.3).margin(0.01));
   CHECK(yb.back() == Approx(ya.back()).margin(1e-4));
}

TEST_CASE("A sustain moved while a note is held reaches the new level")
{
   auto env = q::adsr_envelope_gen{holding_config{}, sps};
   env.attack();
   run(env, std::size_t(sps * 0.5f));            // settled at its default

   env.sustain_level(q::lin_to_db(0.75));
   auto const after = run(env, std::size_t(sps * 0.5f));
   CHECK(after.back() == Approx(0.75).margin(0.01));
}
