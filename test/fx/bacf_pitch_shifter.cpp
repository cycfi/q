/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fx/bacf_pitch_shifter.hpp>
#include <q/fx/transient_detector.hpp>
#include <q/utility/best_lag.hpp>
#include <q_io/audio_file.hpp>

#include <cmath>
#include <numbers>
#include <string>
#include <vector>

namespace q = cycfi::q;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{
   std::vector<float> load(std::string const& name, float& sps)
   {
      q::wav_reader src{"audio_files/" + name + ".wav"};
      REQUIRE(src);
      sps = src.sps();
      std::vector<float> x(src.length());
      src.read(x);
      return x;
   }

   std::vector<float> shift(
      std::vector<float> const& x, float ratio, float sps)
   {
      q::bacf_pitch_shifter shifter{sps};
      std::vector<float> y(x.size());
      for (std::size_t t = 0; t != x.size(); ++t)
         y[t] = shifter(x[t], ratio);
      return y;
   }

   // The frequency of y over [from, to), from its rising zero crossings.
   double frequency(
      std::vector<float> const& y, std::size_t from, std::size_t to
    , double sps)
   {
      double first = -1, last = -1;
      int crossings = 0;
      for (auto t = from + 1; t != to; ++t)
      {
         if (y[t-1] < 0 && y[t] >= 0)
         {
            double at = t - 1 + y[t-1] / (y[t-1] - y[t]);
            if (first < 0)
               first = at;
            last = at;
            ++crossings;
         }
      }
      return (crossings - 1) * sps / (last - first);
   }

   // y before `end`, newest first, as best_lag reads it.
   struct before
   {
      std::vector<float> const& y;
      std::size_t end;
      float operator[](std::size_t i) const { return y[end - 1 - i]; }
   };

   // The period of y just before `end`, searched within a third of
   // `expected`.
   q::lag_result period(
      std::vector<float> const& y, std::size_t end, double expected)
   {
      auto const lo = std::size_t(expected * 2 / 3);
      auto const hi = std::size_t(expected * 4 / 3);
      return q::best_lag(before{y, end}, std::size_t(expected), lo, hi);
   }

   double rms(std::vector<float> const& y, std::size_t from, std::size_t to)
   {
      double sum = 0;
      for (auto t = from; t != to; ++t)
         sum += double(y[t]) * y[t];
      return std::sqrt(sum / (to - from));
   }
}

TEST_CASE("bacf_pitch_shifter: a sine an octave up and down",
   "[bacf_pitch_shifter]")
{
   float const sps = 44100;
   std::vector<float> x(sps * 2);
   for (std::size_t t = 0; t != x.size(); ++t)
      x[t] = 0.5f * float(std::sin(2 * std::numbers::pi * 220 * t / sps));

   auto up = shift(x, 2.0f, sps);
   REQUIRE_THAT(
      frequency(up, sps / 2, x.size(), sps), WithinRel(440, 2e-3));
   REQUIRE_THAT(
      rms(up, sps / 2, x.size()), WithinRel(0.5 / std::sqrt(2), 0.02));

   auto down = shift(x, 0.5f, sps);
   REQUIRE_THAT(
      frequency(down, sps / 2, x.size(), sps), WithinRel(110, 2e-3));
   REQUIRE_THAT(
      rms(down, sps / 2, x.size()), WithinRel(0.5 / std::sqrt(2), 0.02));
}

TEST_CASE("bacf_pitch_shifter: engaged, a change comes out within 10 ms",
   "[bacf_pitch_shifter]")
{
   // A 220 Hz sine steps from 0.25 to 0.5 at 1 s. The step comes out when
   // the output's level, in 1 ms windows, first passes halfway: a shift
   // leaves a sine's level alone.
   float const sps = 44100;
   std::vector<float> x(sps * 2);
   for (std::size_t t = 0; t != x.size(); ++t)
   {
      auto const a = t < sps ? 0.25f : 0.5f;
      x[t] = a * float(std::sin(2 * std::numbers::pi * 220 * t / sps));
   }
   auto const half = 0.375 / std::sqrt(2);
   auto const w = std::size_t(sps / 1000);
   for (float ratio : {0.5f, 0.75f, 1.5f, 2.0f})
   {
      INFO("ratio " << ratio);
      auto y = shift(x, ratio, sps);
      auto t = std::size_t(sps);
      while (t + w < y.size() && rms(y, t, t + w) < half)
         t += w;
      REQUIRE((t - sps) / sps < 0.010);
   }
}

TEST_CASE("bacf_pitch_shifter: a plucked string an octave up and down",
   "[bacf_pitch_shifter]")
{
   // The low E's period, after the pick, against the shifted output's,
   // at four points through the note; the level holds within 1.5 dB.
   float sps;
   auto x = load("1a-Low-E", sps);
   double const in_period = sps / 82.41;
   for (float ratio : {2.0f, 0.5f})
   {
      INFO("ratio " << ratio);
      auto y = shift(x, ratio, sps);
      for (double at : {1.5, 2.0, 3.0, 4.0})
      {
         INFO("at " << at << " s");
         auto const end = std::size_t(at * sps);
         auto in = period(x, end, in_period);
         auto out = period(y, end, in.lag / ratio);
         REQUIRE(out.similarity > 0.9f);
         REQUIRE_THAT(out.lag, WithinRel(in.lag / ratio, 0.01f));
      }
      auto const from = std::size_t(1.2 * sps), to = std::size_t(4.0 * sps);
      auto const gain = 20 * std::log10(rms(y, from, to) / rms(x, from, to));
      REQUIRE_THAT(gain, WithinAbs(0.0, 1.5));
   }
}

TEST_CASE("bacf_pitch_shifter: a pick restarts the read",
   "[bacf_pitch_shifter]")
{
   // From the transient on, the read starts again at the pick's own
   // start, a millisecond before the detection, and moves at the ratio:
   // going down at once, going up once the pick's period is acquired,
   // within latency(). The restart is found where the output follows
   // the pick at the ratio, checked once the restart's crossfade (at most
   // 8 ms) is over, for 4 ms.
   float sps;
   auto x = load("1a-Low-E", sps);
   q::transient_detector detector{sps};
   std::size_t detected = 0;
   for (std::size_t t = 0; t != x.size() && !detected; ++t)
   {
      if (detector(x[t]))
         detected = t;
   }
   REQUIRE_THAT(detected / double(sps), WithinAbs(1.0021, 0.001));

   auto const preroll = std::size_t(0.001 * sps);
   auto read = [&](double at)
   {
      auto const i = std::size_t(at);
      auto const f = float(at - i);
      return x[i] + f * (x[i + 1] - x[i]);
   };
   auto error_from = [&](std::vector<float> const& y, std::size_t start
    , float ratio)
   {
      double error = 0, energy = 0;
      auto const first = std::size_t(0.008 * sps);
      auto const last = std::size_t(0.012 * sps);
      for (auto k = first; k != last; ++k)
      {
         auto const expected = read(detected - preroll + k * ratio);
         auto const e = y[start + k] - expected;
         error += e * e;
         energy += expected * expected;
      }
      return std::sqrt(error / energy);
   };

   q::bacf_pitch_shifter shifter{sps};
   auto const latency = shifter.latency() - preroll;
   REQUIRE_THAT(latency / double(sps), WithinAbs(0.020, 1e-4));

   // Going down: at once.
   auto down = shift(x, 0.5f, sps);
   REQUIRE(error_from(down, detected, 0.5f) < 0.02);

   // Going up: somewhere within the latency, the output follows the pick.
   auto up = shift(x, 2.0f, sps);
   std::size_t restart = 0;
   double best = 1.0;
   for (std::size_t d = 0; d <= latency; ++d)
   {
      auto const e = error_from(up, detected + d, 2.0f);
      if (e < best)
      {
         best = e;
         restart = d;
      }
   }
   REQUIRE(best < 0.02);
   INFO("restart after " << restart / double(sps) * 1000 << " ms");
   REQUIRE(restart >= std::size_t(0.008 * sps));
   REQUIRE(restart <= latency);
}

TEST_CASE("bacf_pitch_shifter: a pick plays once, near unity",
   "[bacf_pitch_shifter]")
{
   // Going up a little (a vibrato near unity), the read sits about a
   // millisecond behind the input, so it plays a pick's onset before the
   // pick is detected; the shifter must not jump back and play it again.
   // Where the read is, at each 0.5 ms after a pick, is the delay that
   // best matches 1.5 ms of the output to the input. A pick played twice
   // is one the read passes, then falls back behind, then passes again.
   float sps;
   auto x = load("GStaccato", sps);
   auto const ratio = std::exp2(0.3f / 12.0f);
   auto y = shift(x, ratio, sps);

   q::transient_detector detector{sps};
   std::vector<std::size_t> picks;
   for (std::size_t t = 0; t != x.size(); ++t)
   {
      if (detector(x[t]))
         picks.push_back(t);
   }
   REQUIRE(picks.size() > 30);

   auto const w = std::size_t(0.0015 * sps);
   auto const max_lag = std::size_t(0.050 * sps);
   auto read_at = [&](std::size_t i) -> long
   {
      double ss = 0;
      for (std::size_t j = 0; j != w; ++j)
         ss += double(y[i + j]) * y[i + j];
      if (ss < 1e-10)
         return -1;
      double best = -2;
      std::size_t lag_at = 0;
      for (std::size_t lag = 0; lag != max_lag; ++lag)
      {
         double xy = 0, xx = 0;
         for (std::size_t j = 0; j != w; ++j)
         {
            auto const r = double(x[i - lag + j]);
            xy += y[i + j] * r;
            xx += r * r;
         }
         auto const c = xy / (std::sqrt(ss * xx) + 1e-12);
         if (c > best)
         {
            best = c;
            lag_at = lag;
         }
      }
      return best > 0.9 ? long(i - lag_at) : -1;
   };

   auto const ms = [&](double v) { return long(v * 0.001 * sps); };
   int twice = 0;
   for (auto p : picks)
   {
      if (p < max_lag || p + std::size_t(ms(45)) >= y.size())
         continue;
      auto const onset = long(p) - ms(1);
      int state = 0;    // before it, past it, back behind it
      for (auto i = long(p) - ms(2); i < long(p) + ms(45); i += ms(0.5))
      {
         auto const r = read_at(std::size_t(i));
         if (r < 0)
            continue;
         if (state == 0 && r >= onset + ms(0.5))
            state = 1;
         else if (state == 1 && r < onset - ms(1))
            state = 2;
         else if (state == 2 && r >= onset + ms(0.5))
         {
            INFO("played twice: the pick at " << p / double(sps) << " s");
            CHECK(false);
            ++twice;
            break;
         }
      }
   }
   CHECK(twice == 0);
}

TEST_CASE("bacf_pitch_shifter: unshifted, the input itself",
   "[bacf_pitch_shifter]")
{
   float sps;
   auto x = load("1a-Low-E", sps);
   auto y = shift(x, 1.0f, sps);
   for (std::size_t t = 0; t != x.size(); ++t)
      REQUIRE(y[t] == x[t]);
}

TEST_CASE("bacf_pitch_shifter: at rest, the next pick plays dry",
   "[bacf_pitch_shifter]")
{
   // A dive from the pick on, back to unison a second later: the read
   // stays behind until the next pick, which returns it to the input
   // within the pre-roll's crossfade.
   float sps;
   auto x = load("1a-Low-E", sps);
   x.resize(std::size_t(3.0 * sps));
   x.insert(x.end(), x.begin(), x.end());

   q::transient_detector detector{sps};
   std::size_t second = 0;
   for (auto t = std::size_t(3.0 * sps); t != x.size() && !second; ++t)
   {
      if (detector(x[t]))
         second = t;
   }
   REQUIRE(second > std::size_t(3.9 * sps));

   q::bacf_pitch_shifter shifter{sps};
   auto const dive = std::size_t(1.0 * sps);
   auto const rest = std::size_t(2.0 * sps);
   std::vector<float> y(x.size());
   for (std::size_t t = 0; t != x.size(); ++t)
      y[t] = shifter(x[t], t >= dive && t < rest ? 0.5f : 1.0f);

   bool behind = false;
   for (auto t = rest; t != rest + std::size_t(0.1 * sps); ++t)
      behind = behind || y[t] != x[t];
   REQUIRE(behind);

   auto const dry = second + std::size_t(0.001 * sps);
   for (auto t = dry; t != x.size(); ++t)
      REQUIRE(y[t] == x[t]);
}

TEST_CASE("bacf_pitch_shifter: reset starts afresh", "[bacf_pitch_shifter]")
{
   float sps;
   auto x = load("1a-Low-E", sps);
   x.resize(std::size_t(2.0 * sps));
   q::bacf_pitch_shifter shifter{sps};
   for (auto s : x)
      shifter(s, 2.0f);
   shifter.reset();
   auto fresh = shift(x, 2.0f, sps);
   for (std::size_t t = 0; t != x.size(); ++t)
      REQUIRE(shifter(x[t], 2.0f) == fresh[t]);
}
