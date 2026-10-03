/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/synth/va/sync.hpp>
#include <q/synth/va/analog_osc.hpp>
#include <q/support/literals.hpp>

#include <cmath>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr auto sps = 48000.0f;

   // A slave sawtooth restarted by a master, and the master's period.
   std::vector<float> synced(double master_hz, double ratio, int n = 8000)
   {
      q::analog_osc osc;
      q::phase_iterator m{q::frequency{master_hz}, sps};
      q::phase_iterator s{q::frequency{master_hz * ratio}, sps};
      q::hard_sync sync;

      std::vector<float> y;
      y.reserve(n);
      for (int i = 0; i != n; ++i)
      {
         ++m;
         ++s;
         sync(s, m);
         y.push_back(osc.saw(s));
      }
      return y;
   }
}

TEST_CASE("hard_sync: the pitch heard is the master's")
{
   // Whatever the slave runs at, the wave repeats with the master, which is
   // the whole point of sync: the slave's frequency becomes a timbre.
   constexpr int period = 128;                  // samples, so 375 Hz
   for (double ratio : {1.3, 2.0, 2.7, 4.4})
   {
      auto y = synced(sps / period, ratio);
      float worst = 0.0f;
      for (std::size_t i = 4000; i != y.size() - period; ++i)
         worst = std::max(worst, std::abs(y[i] - y[i + period]));
      CHECK(worst < 0.02f);
   }
}

TEST_CASE("hard_sync: without it the slave keeps its own period")
{
   // The same run with no sync does not repeat at the master's period,
   // which is what makes the check above meaningful.
   constexpr int period = 128;
   q::analog_osc osc;
   q::phase_iterator s{q::frequency{sps / period * 2.7}, sps};

   std::vector<float> y;
   for (int i = 0; i != 8000; ++i, ++s)
      y.push_back(osc.saw(s));

   float worst = 0.0f;
   for (std::size_t i = 4000; i != y.size() - period; ++i)
      worst = std::max(worst, std::abs(y[i] - y[i + period]));
   CHECK(worst > 0.5f);
}

TEST_CASE("hard_sync: the restart lands between samples")
{
   // The master is some fraction of a step past zero when it wraps, and the
   // slave starts the same fraction into its own step.
   q::phase_iterator m{1000_Hz, sps};
   q::phase_iterator s{1777_Hz, sps};
   q::hard_sync sync;

   int resets = 0;
   for (int i = 0; i != 2000; ++i)
   {
      ++m;
      ++s;
      if (sync(s, m))
      {
         ++resets;
         auto f = sync.fraction();
         CHECK(f >= 0.0f);
         CHECK(f <= 1.0f);
         CHECK(float(s._phase.rep) / s._step.rep == Approx(f).margin(0.01f));
      }
   }
   CHECK(resets == Approx(2000.0f * 1000 / sps).margin(1.0f));
}

TEST_CASE("hard_sync: it leaves the slave alone mid-cycle")
{
   q::phase_iterator m{100_Hz, sps};
   q::phase_iterator s{700_Hz, sps};
   q::hard_sync sync;

   ++m;
   ++s;
   sync(s, m);                                  // the first call arms it

   int quiet = 0;
   for (int i = 0; i != 200; ++i)               // well inside one master cycle
   {
      ++m;
      ++s;
      auto before = s._phase;
      if (!sync(s, m))
      {
         CHECK(s._phase.rep == before.rep);
         ++quiet;
      }
   }
   CHECK(quiet == 200);
}
