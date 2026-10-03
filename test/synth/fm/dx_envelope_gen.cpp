/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/synth/concepts.hpp>
#include <q/synth/fm/dx_envelope_gen.hpp>
#include <cmath>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;
using eg = q::dx_envelope_gen;
using config = eg::config;

static_assert(q::concepts::EnvelopeGenerator<eg>);

namespace
{
   constexpr float sps = 48000.0f;
   constexpr auto silent = eg::floor;
   constexpr auto quick = eg::fastest;     // the whole range at once

   // A rate: the time the whole range takes, from a speed in dB a second
   constexpr q::duration rate_of(double db_per_second)
   {
      return q::duration{eg::range.rep / db_per_second};
   }

   std::vector<float> run(eg& e, std::size_t n)
   {
      std::vector<float> y(n);
      for (auto& v : y)
         v = e();
      return y;
   }

   // Samples until the envelope reaches segment i, or n
   std::size_t samples_until(eg& e, std::size_t i, std::size_t n)
   {
      for (std::size_t k = 0; k != n; ++k)
      {
         if (e.index() == i)
            return k;
         e();
      }
      return n;
   }

   float db(float gain)
   {
      return 20.0f * std::log10(gain);
   }

   float gain(float db)
   {
      return std::pow(10.0f, db / 20);
   }

   // Seconds for an attack at a speed, from attack_from to a target: the
   // law integrated in fine steps, the speed times 2 plus the doublings
   // below full
   double attack_seconds(double db_per_second, double target)
   {
      double t = 0;
      for (double d = eg::attack_from.rep; d < target; d += 0.001)
      {
         double factor = std::max(1.0, 2 + std::floor(-d / 6.0206));
         t += 0.001 / (db_per_second * factor);
      }
      return t;
   }
}

TEST_CASE("Segments run in order to their levels, then hold")
{
   eg e{{.rate = {quick, quick, quick, quick}
      , .level = {0_dB, -20_dB, -10_dB, silent}}, sps};
   CHECK(e.in_idle_phase());
   CHECK(e() == 0.0f);

   e.attack();
   CHECK(e.index() == eg::r1);
   CHECK(samples_until(e, eg::r2, 48000) < 500);
   CHECK(e.level() == 0.0f);

   CHECK(samples_until(e, eg::r3, 48000) < 500);
   CHECK(e.level() == -20.0f);

   CHECK(samples_until(e, eg::sustain, 48000) < 500);
   CHECK(e.level() == -10.0f);

   // Holds at L3 indefinitely
   for (auto g : run(e, 48000))
      REQUIRE(g == Approx(gain(-10)).epsilon(1e-4));
   CHECK(e.index() == eg::sustain);

   e.release();
   CHECK(e.in_release_phase());
   CHECK(samples_until(e, eg::idle, 48000) < 500);
   CHECK(e() == 0.0f);
}

TEST_CASE("A rate is the time the whole range would take")
{
   // Half the range takes half the time: the rate is a slope
   for (double seconds : {0.1, 0.5, 2.0})
   {
      eg e{{.rate = {quick, q::duration{seconds}, quick, quick}
         , .level = {0_dB, silent, silent, silent}}, sps};
      e.attack();
      samples_until(e, eg::r2, 1000);

      auto half = std::size_t(seconds / 2 * sps);
      auto y = run(e, half + 1);
      CHECK(db(y[0]) - db(y[half])
         == Approx(eg::range.rep / 2).epsilon(0.01));
   }
}

TEST_CASE("A decay falls at its rate's speed in dB a second")
{
   for (double speed : {50.0, 200.0, 1000.0, 5000.0})
   {
      eg e{{.rate = {quick, rate_of(speed), quick, quick}
         , .level = {0_dB, silent, silent, silent}}, sps};
      e.attack();
      samples_until(e, eg::r2, 1000);

      auto n = std::size_t(20.0 / speed * sps);          // 20 dB
      auto y = run(e, n + 1);
      CHECK(db(y[0]) - db(y[n]) == Approx(20.0f).epsilon(0.01));
   }
}

TEST_CASE("An attack climbs faster the lower it is, and slows near full")
{
   eg a{{.rate = {rate_of(100), quick, quick, quick}
      , .level = {0_dB, 0_dB, 0_dB, silent}}, sps};
   a.attack();
   CHECK(a.level() == Approx(eg::attack_from.rep));   // from silence
   auto t = samples_until(a, eg::sustain, 48000 * 10);
   CHECK(t == Approx(attack_seconds(100, 0) * sps).epsilon(0.01));

   // A lower target is quicker, by more than the dB ratio
   eg b{{.rate = {rate_of(100), quick, quick, quick}
      , .level = {-24_dB, -24_dB, -24_dB, silent}}, sps};
   b.attack();
   auto tb = samples_until(b, eg::sustain, 48000 * 10);
   CHECK(tb == Approx(attack_seconds(100, -24) * sps).epsilon(0.02));
   CHECK(tb < t / 2.5);

   // The early climb is steeper in dB than the late one
   eg c{{.rate = {rate_of(100), quick, quick, quick}
      , .level = {0_dB, 0_dB, 0_dB, silent}}, sps};
   c.attack();
   auto y = run(c, t);
   auto early = db(y[t / 4]) - db(y[1]);
   auto late = db(y[t - 1]) - db(y[3 * t / 4]);
   CHECK(early > 3 * late);
}

TEST_CASE("Release from any segment falls at R4 from where it is")
{
   config cfg{
      .rate = {rate_of(60), rate_of(60), rate_of(60), rate_of(200)}
    , .level = {0_dB, -30_dB, -12_dB, silent}};

   for (auto seg : {eg::r1, eg::r2, eg::r3})
   {
      eg e{cfg, sps};
      e.attack();
      samples_until(e, seg, 48000 * 60);
      run(e, 4800);
      auto g = e();
      auto level = e.level();

      e.release();
      CHECK(e.in_release_phase());
      CHECK(e.level() == level);
      auto y = run(e, 1);
      CHECK(y[0] < g);                    // falls right away, no jump
      CHECK(y[0] > g * 0.99f);

      auto expect = (level - silent.rep) / 200 * sps;
      CHECK(samples_until(e, eg::idle, 48000 * 60) + 1
         == Approx(expect).epsilon(0.01));
   }
}

TEST_CASE("Release holds at L4 when L4 is audible")
{
   eg e{{.rate = {quick, quick, quick, quick}
      , .level = {0_dB, 0_dB, 0_dB, -20_dB}}, sps};
   e.attack();
   samples_until(e, eg::sustain, 48000);
   e.release();
   run(e, 48000);
   CHECK(e.in_release_phase());
   CHECK(e.level() == -20.0f);
   CHECK(e() == Approx(gain(-20)).epsilon(1e-4));
}

TEST_CASE("Retrigger restarts from the current level without a jump")
{
   eg e{{.rate = {rate_of(300), quick, quick, rate_of(300)}
      , .level = {0_dB, 0_dB, 0_dB, silent}}, sps};
   e.attack();
   samples_until(e, eg::sustain, 48000 * 60);
   e.release();
   run(e, 4800);
   auto before = e();

   e.attack();
   CHECK(e.index() == eg::r1);
   auto after = e();
   CHECK(after > before);
   CHECK(after - before < 0.01f * before);

   // and the attack completes to L1
   samples_until(e, eg::sustain, 48000 * 60);
   CHECK(e.level() == 0.0f);
}

TEST_CASE("Levels below the floor are silent")
{
   auto under = q::dB(silent.rep - 50);
   eg e{{.rate = {quick, quick, quick, quick}
      , .level = {0_dB, under, under, silent}}, sps};
   e.attack();
   samples_until(e, eg::sustain, 48000);
   CHECK(e.level() == Approx(silent.rep));   // clamped to the floor
   CHECK(e() == 0.0f);
}

TEST_CASE("The gain is a master volume on the output")
{
   eg e{{.rate = {quick, quick, quick, quick}
      , .level = {-20_dB, -20_dB, -20_dB, silent}}, sps};
   e.attack();
   samples_until(e, eg::sustain, 48000);
   CHECK(e.gain() == 1.0f);
   e.gain(0.5f);
   CHECK(e() == Approx(0.5f * gain(-20)).epsilon(1e-4));
   CHECK(e.level() == -20.0f);              // the level is untouched
}

TEST_CASE("Reset idles the envelope, silent")
{
   eg e{{.rate = {quick, quick, quick, quick}
      , .level = {0_dB, 0_dB, 0_dB, silent}}, sps};
   e.attack();
   run(e, 1000);
   e.reset();
   CHECK(e.in_idle_phase());
   CHECK(e() == 0.0f);
   CHECK(e.level() == Approx(silent.rep));
}
