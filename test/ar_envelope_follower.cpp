/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Contract tests for the ar_envelope_follower: one-pole tracking toward
// the input with separate attack and release coefficients, both on the
// -2/(sps*d) convention — a unit step reaches 1-e^-2 (~0.865) after the
// attack duration; a step to silence decays to e^-2 (~0.135) after the
// release duration.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/fx/envelope.hpp>
#include <cmath>

namespace q = cycfi::q;
using namespace q::literals;

constexpr float sps = 48000.0f;

TEST_CASE("ar: unit step reaches 1 - e^-2 after the attack duration")
{
   constexpr float attack_s = 0.01f;
   auto env = q::ar_envelope_follower{q::duration{attack_s}, 100_ms, sps};

   float v = 0.0f;
   auto n = std::size_t(attack_s * sps);
   for (std::size_t i = 0; i != n; ++i)
      v = env(1.0f);
   CHECK(v == Approx(1.0f - std::exp(-2.0f)).epsilon(0.02));
}

TEST_CASE("ar: step to silence decays to e^-2 after the release duration")
{
   constexpr float release_s = 0.05f;
   auto env = q::ar_envelope_follower{1_ms, q::duration{release_s}, sps};

   env = 1.0f;                      // seed at the peak
   float v = 1.0f;
   auto n = std::size_t(release_s * sps);
   for (std::size_t i = 0; i != n; ++i)
      v = env(0.0f);
   CHECK(v == Approx(std::exp(-2.0f)).epsilon(0.02));
}

TEST_CASE("ar: attack and release are independent directions")
{
   // Fast attack, slow release: rises much faster than it falls.
   auto env = q::ar_envelope_follower{1_ms, 100_ms, sps};

   float v = 0.0f;
   for (int i = 0; i != 48; ++i)    // 1 ms of full scale
      v = env(1.0f);
   float const after_attack = v;
   CHECK(after_attack > 0.8f);      // ~0.865 at 1x attack

   for (int i = 0; i != 48; ++i)    // 1 ms of silence
      v = env(0.0f);
   // 1 ms into a 100 ms release: barely moved (e^-0.02 ~ 0.98)
   CHECK(v > 0.95f * after_attack);
}

TEST_CASE("ar: seeding and getter")
{
   auto env = q::ar_envelope_follower{10_ms, 50_ms, sps};
   env = 0.5f;
   CHECK(env() == 0.5f);
}

TEST_CASE("ar: config() and attack()/release() mutators rescale the rates")
{
   auto a = q::ar_envelope_follower{10_ms, 10_ms, sps};
   auto b = q::ar_envelope_follower{99_s, 99_s, sps};
   b.config(10_ms, 10_ms, sps);     // now identical to a

   float va = 0.0f, vb = 0.0f;
   for (int i = 0; i != 480; ++i)
   {
      va = a(1.0f);
      vb = b(1.0f);
   }
   CHECK(vb == Approx(va));

   // attack()/release() take plain float seconds
   auto c = q::ar_envelope_follower{99_s, 99_s, sps};
   c.attack(0.01f, sps);
   float vc = 0.0f;
   for (int i = 0; i != 480; ++i)
      vc = c(1.0f);
   CHECK(vc == Approx(va).epsilon(0.001));
}
