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
#include <utility>
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

TEST_CASE("bacf_lag_tracker: a lag that drifts out of range drops the lock")
{
   // 400 Hz (120 samples) for 0.2 s, a glide up to 600 Hz (80) over 0.5 s,
   // past the 96-sample floor, then 600 Hz: the lock must not hold the
   // last lag it could score. It locks again on a multiple of the new
   // period.
   double const period = sps / 600.0;
   std::size_t const hold = std::size_t(0.2 * sps)
    , glide = std::size_t(0.5 * sps)
    , end = hold + glide + std::size_t(0.4 * sps);
   q::bacf_lag_tracker tr{2_ms, 45_ms, sps};
   double phase = 0;
   bool lost = false;
   std::size_t on = 0, locked = 0;
   for (std::size_t i = 0; i != end; ++i)
   {
      double const f = i < hold ? 400.0
         : i < hold + glide ? 400.0 + 200.0 * (i - hold) / glide : 600.0;
      phase += 2 * pi * f / sps;
      bool const l = tr(0.5f * float(std::sin(phase)));
      if (i == hold)
         REQUIRE(l);
      if (i > hold && !l)
         lost = true;
      if (i >= end - std::size_t(0.1 * sps) && l)
      {
         ++locked;
         on += on_multiple(tr.lag(), period);
      }
   }
   CHECK(lost);
   CHECK(locked == std::size_t(0.1 * sps));
   CHECK(on == locked);
}

namespace
{
   // A sine for 0.2 s, then a drop with `hint`: the samples from the drop
   // to the first provisional lag, and that lag.
   std::pair<std::size_t, float> provisional_after(double freq, float hint)
   {
      q::bacf_lag_tracker tr{2_ms, 45_ms, sps};
      auto tone = [=](std::size_t i)
         { return 0.5f * float(std::sin(2 * pi * freq * i / sps)); };
      std::size_t const at = std::size_t(0.2 * sps);
      for (std::size_t i = 0; i != at; ++i)
         tr(tone(i));
      tr.drop(hint);
      REQUIRE(tr.provisional() == 0.0f);
      for (std::size_t i = at; i != at + std::size_t(0.1 * sps); ++i)
      {
         tr(tone(i));
         if (tr.provisional() > 0.0f)
            return {i - at, tr.provisional()};
      }
      return {0, 0.0f};
   }
}

TEST_CASE("bacf_lag_tracker: a provisional lag after a drop")
{
   // The first pair of like pulses after the drop gives the period, about
   // two periods in.
   double const period = sps / 196.0;
   auto const plain = provisional_after(196, 0.0f);
   REQUIRE(plain.second > 0.0f);
   CHECK(on_multiple(plain.second, period));
   CHECK(plain.first < std::size_t(2.5 * period));
}

TEST_CASE("bacf_lag_tracker: after a long hint, a provisional lag is vetted")
{
   // 600 Hz is under the floor, so the estimate is two periods (160
   // samples). After a long hint (a chord) the estimate stands at once if
   // the hint is a clean multiple of it, else only once it has repeated
   // over one lag, which comes later.
   double const period = sps / 600.0;
   auto const plain = provisional_after(600, 0.0f);
   REQUIRE(plain.second > 0.0f);
   CHECK(on_multiple(plain.second, period));

   auto const agrees = provisional_after(600, 4 * plain.second);
   CHECK(agrees.first == plain.first);
   CHECK(agrees.second == plain.second);

   auto const other = provisional_after(600, float(0.017 * sps));
   CHECK(other.second == plain.second);
   CHECK(other.first > plain.first);
}

TEST_CASE("bacf_lag_tracker: a provisional lag near the longest is vetted")
{
   // After a long hint, an estimate near the longest lag is vetted over one
   // lag of it: the history must hold that lag, one lag of it and a word.
   // 22.7 Hz repeats every 2115 samples, under the 45 ms (2160) longest; the
   // hint, 1440, is not a clean multiple of it.
   double const freq = 22.7;
   double const period = sps / freq;
   q::bacf_lag_tracker tr{2_ms, 45_ms, sps};
   auto tone = [&](std::size_t i)
      { return 0.5f * float(std::sin(2 * pi * freq * i / sps)); };
   std::size_t const at = std::size_t(0.5 * sps);
   for (std::size_t i = 0; i != at; ++i)
      tr(tone(i));
   tr.drop(1440.0f);
   float prov = 0;
   for (std::size_t i = at; i != at + std::size_t(0.5 * sps) && !prov; ++i)
   {
      tr(tone(i));
      prov = tr.provisional();
   }
   REQUIRE(prov > 0.0f);
   CHECK(on_multiple(prov, period));
}

TEST_CASE("bacf_lag_tracker: a pick's provisional lag comes before its lock")
{
   q::wav_reader src{"audio_files/1a-Low-E.wav"};
   REQUIRE(src);
   float const fs = src.sps();
   std::vector<float> in(src.length());
   src.read(in);
   float peak = 0;
   for (auto s : in)
      peak = std::max(peak, std::abs(s));
   std::size_t pick = 0;
   while (std::abs(in[pick]) < 0.1f * peak)
      ++pick;

   q::bacf_lag_tracker tr{2_ms, 45_ms, fs};
   q::lowpass lp{1_kHz, fs};
   double const period = fs / 82.41;
   std::size_t first_prov = 0, first_lock = 0;
   float prov = 0;
   for (std::size_t i = 0; i != pick + std::size_t(0.1 * fs); ++i)
   {
      if (i == pick)
         tr.drop();
      bool const l = tr(lp(in[i]));
      if (i < pick)
         continue;
      if (!first_prov && tr.provisional() > 0.0f)
      {
         first_prov = i - pick;
         prov = tr.provisional();
      }
      if (!first_lock && l)
         first_lock = i - pick;
   }
   INFO("provisional at " << first_prov / fs * 1000 << " ms, lock at "
      << first_lock / fs * 1000 << " ms");
   REQUIRE(prov > 0.0f);
   CHECK(on_multiple(prov, period, 0.07));
   CHECK(first_prov < first_lock);
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
