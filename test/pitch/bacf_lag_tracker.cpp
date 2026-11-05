/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/pitch/bacf_lag_tracker.hpp>
#include <q/fx/biquad.hpp>
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>

#include <cmath>
#include <string>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr float sps = 48000;

   // The lag is a whole number of periods, within `tol` periods.
   bool on_multiple(float lag, double period, double tol = 0.02)
   {
      auto const k = std::round(lag / period);
      return k >= 1 && std::abs(lag / period - k) < tol;
   }

   struct run_result
   {
      std::size_t first_lock = 0;   // samples
      std::size_t locked = 0;       // samples locked after the first lock
      std::size_t on_period = 0;    // of those, on a multiple
      std::size_t samples = 0;      // after the first lock
      float       max_error = 0;    // from `period` itself, locked
   };

   template <typename Gen>
   run_result run(q::bacf_lag_tracker& tr, Gen gen, std::size_t n
    , double period)
   {
      run_result r;
      bool started = false;
      for (std::size_t i = 0; i != n; ++i)
      {
         bool const l = tr(gen(i));
         if (l && !started)
         {
            started = true;
            r.first_lock = i;
         }
         if (!started)
            continue;
         ++r.samples;
         if (l)
         {
            ++r.locked;
            r.on_period += on_multiple(tr.lag(), period);
            r.max_error = std::max(r.max_error
             , float(std::abs(tr.lag() - period)));
         }
      }
      return r;
   }

   double const pi = 3.14159265358979;
}

TEST_CASE("bacf_lag_tracker: locks on a sine, to a fraction of a sample")
{
   // 196 Hz: 244.9 samples, not a whole number.
   double const f = 196.0, period = sps / f;
   q::bacf_lag_tracker tr{2_ms, 45_ms, sps};
   auto r = run(tr, [&](std::size_t i)
      { return 0.5f * float(std::sin(2 * pi * f * i / sps)); }
    , std::size_t(sps), period);

   INFO("first lock " << r.first_lock << " samples");
   CHECK(r.first_lock < std::size_t(0.030 * sps));
   CHECK(r.locked == r.samples);
   CHECK(r.on_period == r.locked);
   CHECK(r.max_error < 0.5f);
   CHECK(tr.periodicity() > 0.95f);
   CHECK(tr.window() % q::bitset<>::value_size == 0);
}

TEST_CASE("bacf_lag_tracker: a harmonic tone locks on its period")
{
   // 110 Hz, five harmonics falling 6 dB each.
   double const f = 110.0, period = sps / f;
   q::bacf_lag_tracker tr{2_ms, 45_ms, sps};
   auto r = run(tr, [&](std::size_t i)
      {
         double s = 0, a = 0.5;
         for (int h = 1; h <= 5; ++h, a *= 0.5)
            s += a * std::sin(2 * pi * f * h * i / sps);
         return float(s);
      }
    , std::size_t(sps), period);

   CHECK(r.first_lock < std::size_t(0.050 * sps));
   CHECK(r.locked == r.samples);
   CHECK(r.on_period == r.locked);
}

TEST_CASE("bacf_lag_tracker: two notes lock on their common period")
{
   // 200 and 300 Hz repeat every 10 ms (480 samples); neither note's own
   // period does.
   double const period = sps / 100.0;
   q::bacf_lag_tracker tr{2_ms, 45_ms, sps};
   auto r = run(tr, [&](std::size_t i)
      {
         return float(0.4 * std::sin(2 * pi * 200 * i / sps)
            + 0.4 * std::sin(2 * pi * 300 * i / sps));
      }
    , std::size_t(sps), period);

   CHECK(r.locked > 0);
   CHECK(r.on_period == r.locked);
}

TEST_CASE("bacf_lag_tracker: holds through a loss, then locks again")
{
   // 196 Hz for half a second, then 330 Hz.
   double const p1 = sps / 196.0, p2 = sps / 330.0;
   std::size_t const change = std::size_t(0.5 * sps);
   q::bacf_lag_tracker tr{2_ms, 45_ms, sps};
   auto tone = [&](std::size_t i)
   {
      double const f = i < change ? 196.0 : 330.0;
      return 0.5f * float(std::sin(2 * pi * f * i / sps));
   };

   bool lost = false, relocked = false;
   float held = 0, last = 0;
   for (std::size_t i = 0; i != std::size_t(sps); ++i)
   {
      bool const l = tr(tone(i));
      if (l)
         last = tr.lag();
      if (i < change)
      {
         if (i > change / 2)
         {
            REQUIRE(l);
            held = tr.lag();
         }
         continue;
      }
      if (!l && !relocked)
      {
         lost = true;
         CHECK(tr.lag() == last);      // the last good lag, held
      }
      if (l && lost)
         relocked = true;
      if (i > change + std::size_t(0.1 * sps))
      {
         CHECK(l);
         CHECK(on_multiple(tr.lag(), p2));
      }
   }
   CHECK(on_multiple(held, p1));
   CHECK(lost);
   CHECK(relocked);
}

TEST_CASE("bacf_lag_tracker: drop and reset")
{
   q::bacf_lag_tracker tr{2_ms, 45_ms, sps};
   for (std::size_t i = 0; i != std::size_t(0.2 * sps); ++i)
      tr(0.5f * float(std::sin(2 * pi * 196 * i / sps)));
   REQUIRE(tr.locked());
   auto const lag = tr.lag();

   tr.drop();
   CHECK(!tr.locked());
   CHECK(tr.lag() == lag);

   tr.reset();
   CHECK(!tr.locked());
   CHECK(tr.lag() == 0.0f);
}

TEST_CASE("bacf_lag_tracker: stays locked over a long run")
{
   // Ten seconds: the bitstream slides many times over.
   double const period = sps / 196.0;
   q::bacf_lag_tracker tr{2_ms, 45_ms, sps};
   auto r = run(tr, [&](std::size_t i)
      { return 0.5f * float(std::sin(2 * pi * 196 * i / sps)); }
    , std::size_t(10 * sps), period);
   CHECK(r.locked == r.samples);
   CHECK(r.max_error < 0.5f);
}

namespace
{
   // A recording through a 1 kHz low-pass, from 0.2 s after its pick to
   // 1.5 s: the share of samples locked on a multiple of the period.
   float on_period_share(std::string name, double freq)
   {
      q::wav_reader src{"audio_files/" + name + ".wav"};
      REQUIRE(src);
      float const fs = src.sps();
      std::vector<float> in(src.length());
      src.read(in);

      // The pick: where the level first reaches a tenth of the peak.
      float peak = 0;
      for (auto s : in)
         peak = std::max(peak, std::abs(s));
      std::size_t pick = 0;
      while (std::abs(in[pick]) < 0.1f * peak)
         ++pick;

      q::bacf_lag_tracker tr{5_ms, 45_ms, fs};
      q::lowpass lp{1_kHz, fs};
      double const period = fs / freq;
      std::size_t const from = pick + std::size_t(0.2 * fs);
      std::size_t const to = pick + std::size_t(1.5 * fs);
      std::size_t good = 0;
      for (std::size_t i = 0; i != to; ++i)
      {
         bool const l = tr(lp(in[i]));
         if (i >= from)
            good += l && on_multiple(tr.lag(), period);
      }
      return float(good) / float(to - from);
   }
}

TEST_CASE("bacf_lag_tracker: guitar single notes")
{
   CHECK(on_period_share("1a-Low-E", 82.41) > 0.95f);
   CHECK(on_period_share("3a-D", 146.83) > 0.95f);
   CHECK(on_period_share("4a-G", 196.0) > 0.95f);
}
