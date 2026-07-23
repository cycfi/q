/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/fx/peak_picker.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/signal_conditioner.hpp>
#include <q_io/audio_file.hpp>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

#include "golden_csv.hpp"

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr float pi = 3.14159265358979323846f;

   struct mark { std::uint32_t pos; float amp; };

   // Drive `chain` (a float -> q::pick callable: the picker, wrapped in zero or
   // more qualifiers) over a buffer, collecting every confirmed peak. The
   // picker carries no sample index, so the caller stamps `pos` from its loop.
   template <typename Chain>
   std::vector<mark> run(Chain chain, std::vector<float> const& in)
   {
      std::vector<mark> peaks;
      for (std::size_t i = 0; i != in.size(); ++i)
      {
         auto r = chain(in[i]);
         if (r.hit)
            peaks.push_back({std::uint32_t(i), r.info.amp});
      }
      return peaks;
   }

   // A pure sine: `cycles` periods at `freq`, unit amplitude.
   std::vector<float> make_sine(float freq, float sps, int cycles)
   {
      auto const n = std::size_t((sps / freq) * cycles);
      std::vector<float> v(n);
      for (std::size_t i = 0; i != n; ++i)
         v[i] = std::sin(2 * pi * freq * i / sps);
      return v;
   }
}

///////////////////////////////////////////////////////////////////////////////
// Tier 1 -- synthetic unit tests. Deterministic, no conditioner: these pin
// the core mechanism and the functional qualifier chain.
///////////////////////////////////////////////////////////////////////////////

TEST_CASE("peak_picker: pure sine -> one pick per cycle at the crest")
{
   float const sps = 48000, freq = 440;
   float const period = sps / freq;
   int const cycles = 20;
   auto in = make_sine(freq, sps, cycles);

   q::peak_picker pk;                               // bare: every local max
   auto peaks = run([&](float s){ return pk(s); }, in);

   // One crest per cycle (a whole-cycle buffer may clip the last crest).
   CHECK(peaks.size() >= std::size_t(cycles - 1));
   CHECK(peaks.size() <= std::size_t(cycles));

   // The apex is the true crest: amp ~ 1, spacing ~ one period, first crest a
   // quarter period in.
   CHECK(float(peaks.front().pos) == Approx(period / 4).margin(1.5f));
   for (auto const& p : peaks)
      CHECK(p.amp == Approx(1.0f).margin(0.01f));
   for (std::size_t k = 1; k != peaks.size(); ++k)
      CHECK(float(peaks[k].pos - peaks[k - 1].pos) == Approx(period).margin(1.5f));
}

TEST_CASE("peak_picker: peak_gate at level 0 keeps only above-zero peaks")
{
   // A level of 0 makes the bar `ratio * 0 == 0`, so the strict test is exactly
   // `amp > 0`: this is what peak_positive used to be.
   q::peak_info st;
   q::peak_gate g{1.0f};
   auto feed = [&](float amp, bool hit)
   {
      st.amp = amp;
      return g(q::pick{hit, st}, 0.0f).hit;
   };

   CHECK(feed(0.5f, true));                          // positive apex -> kept
   CHECK_FALSE(feed(-0.3f, true));                   // negative apex -> dropped
   CHECK_FALSE(feed(0.0f, true));                    // exactly zero -> dropped
   CHECK_FALSE(feed(0.9f, false));                   // not a peak -> nothing
}

TEST_CASE("peak_picker: peak_gate on a peak-envelope keeps decaying crests")
{
   float const sps = 48000, freq = 300;
   int const cycles = 24;
   auto in = make_sine(freq, sps, cycles);
   // Shrink the crests toward the tail: a FIXED floor would drop the quiet end,
   // but the adaptive bar tracks the note down and keeps marking every cycle.
   float const decay = std::pow(0.3f, 1.0f / in.size());   // -> 0.3 over the run
   for (std::size_t i = 0; i != in.size(); ++i)
      in[i] *= std::pow(decay, float(i));

   q::peak_picker pk;
   q::peak_gate gate{0.6f};                          // 0.6 x running envelope
   q::peak_envelope_follower env{q::frequency{freq}.period() * 4, sps};
   auto peaks = run([&](float s){ return gate(pk(s), env(std::abs(s))); }, in);

   CHECK(peaks.size() >= std::size_t(cycles - 1));
   for (std::size_t k = 1; k != peaks.size(); ++k)
      CHECK(peaks[k].amp < peaks[k - 1].amp);        // strictly decaying
   CHECK(peaks.back().amp < 0.5f);                   // quiet tail, still picked
}

TEST_CASE("peak_picker: two peak_gates chain to keep the crest")
{
   // A band-limited sawtooth: one dominant crest per cycle, with small Gibbs
   // ripples on the slopes and in the negative excursion. A peak_gate at level
   // 0 drops the negative-going ripple maxima; a second peak_gate on the
   // running envelope drops the low ripples under a fraction of it. Only the
   // dominant crest survives -- two gates, two injected levels.
   float const sps = 48000, f0 = 250;
   float const period = sps / f0;
   int const cycles = 16;
   auto const n = std::size_t(period * cycles);
   std::vector<float> in(n);
   for (std::size_t i = 0; i != n; ++i)
   {
      float t = float(i) / sps, s = 0.0f;
      for (int k = 1; k != 6; ++k)
         s += std::sin(2 * pi * k * f0 * t) / k;
      in[i] = s;
   }

   q::peak_picker pk1;
   auto bare = run([&](float s){ return pk1(s); }, in);

   q::peak_picker pk2;
   q::peak_gate pos{1.0f};                           // level 0 -> above-zero
   q::peak_gate gate{0.6f};                          // 0.6 x running envelope
   q::peak_envelope_follower env{q::frequency{f0}.period() * 4, sps};
   auto kept = run(
      [&](float s){ return gate(pos(pk2(s), 0.0f), env(std::abs(s))); }, in);

   CHECK(bare.size() > kept.size());                // ripple maxima existed & cut
   CHECK(bare.size() > std::size_t(cycles));         // (self-check: multi-max)
   for (auto const& p : kept)
      CHECK(p.amp > 0.0f);                           // level 0 did its job
   CHECK(kept.size() >= std::size_t(cycles - 1));
   CHECK(kept.size() <= std::size_t(cycles + 1));
}

TEST_CASE("peak_picker: peak_min_slope rejects slowly-varying peaks")
{
   // Two sines at the same amplitude but very different rates. The gentle one
   // barely moves over the slope window; the sharp one climbs steeply into
   // each crest. peak_min_slope keeps the sharp peaks and drops the gentle
   // ones, purely on rate, independent of amplitude. The host owns the slope
   // and injects it.
   float const sps = 48000;
   auto slow = make_sine(50, sps, 8);      // gentle: low slope
   auto fast = make_sine(800, sps, 8);     // sharp: high slope

   auto keep = [&](std::vector<float> const& in)
   {
      q::peak_picker pk;
      q::peak_min_slope ms{0.5f};
      q::slope sl{0.5_ms, sps};
      return run([&](float s){ return ms(pk(s), sl(s)); }, in);
   };

   CHECK(keep(fast).size() >= 7);          // sharp peaks pass
   CHECK(keep(slow).empty());              // gentle peaks rejected
}

TEST_CASE("peak_picker: peak_z_score keeps peaks that stand out from noise")
{
   // A low baseline oscillation with tall isolated pulses on top. The baseline
   // crests never rise far above the running mean; the pulses do. z_score keeps
   // the pulses and rejects the baseline.
   float const sps = 48000;
   int const n = 6000;
   std::vector<float> in(n);
   for (int i = 0; i != n; ++i)
      in[i] = 0.08f * std::sin(2 * pi * 250 * i / sps);
   int pulses = 0;
   for (int c = 700; c < n; c += 1000, ++pulses)
      for (int k = -40; k <= 40; ++k)
         in[c + k] += 0.5f * (1.0f + std::cos(pi * k / 40));

   q::peak_picker pk;
   q::peak_z_score z{3.0f, 0.1f, 20_ms, sps};
   auto kept = run([&](float s){ return z(pk(s)); }, in);

   int tall = 0;                                       // apex above the pulses' foot
   for (auto const& p : kept)
      if (p.amp > 0.5f) ++tall;
   CHECK(tall == pulses);                              // every pulse kept
   CHECK(kept.size() - std::size_t(tall) <= 3);        // only startup crests slip
}

TEST_CASE("peak_picker: peak_gate on host RMS keeps one crest per cycle")
{
   // A band-limited sawtooth: one dominant crest per cycle with small ripples.
   // The RMS over one period is a stable level the crest stands well above and
   // the ripples do not, so the gate keeps one landmark per cycle. The host
   // owns the RMS follower and injects its output.
   float const sps = 48000, f0 = 250;
   float const period = sps / f0;
   int const cycles = 16;
   auto const n = std::size_t(period * cycles);
   std::vector<float> in(n);
   for (std::size_t i = 0; i != n; ++i)
   {
      float t = float(i) / sps, s = 0.0f;
      for (int k = 1; k != 6; ++k)
         s += std::sin(2 * pi * k * f0 * t) / k;
      in[i] = s;
   }

   q::peak_picker pk;
   q::peak_gate rg{1.6f};
   q::true_rms_envelope_follower rms{q::frequency{f0}.period(), sps};
   auto kept = run([&](float s){ return rg(pk(s), rms(s)); }, in);

   CHECK(kept.size() >= std::size_t(cycles - 2));   // about one per cycle
   CHECK(kept.size() <= std::size_t(cycles + 1));
   for (auto const& p : kept)
      CHECK(p.amp > 0.0f);
}

TEST_CASE("peak_picker: peak_gate on host mean-|s| keeps one crest per cycle")
{
   // The same periodic gate on a moving average of |s| instead of the RMS --
   // cheaper (no square, no sqrt) and, since the mean of |s| runs under the
   // RMS, tuned with a slightly higher ratio for the same one-per-cycle result.
   float const sps = 48000, f0 = 250;
   float const period = sps / f0;
   int const cycles = 16;
   auto const n = std::size_t(period * cycles);
   std::vector<float> in(n);
   for (std::size_t i = 0; i != n; ++i)
   {
      float t = float(i) / sps, s = 0.0f;
      for (int k = 1; k != 6; ++k)
         s += std::sin(2 * pi * k * f0 * t) / k;
      in[i] = s;
   }

   q::peak_picker pk;
   q::peak_gate mg{1.8f};
   q::moving_average ma{q::frequency{f0}.period(), sps};
   auto kept = run([&](float s){ return mg(pk(s), ma(std::abs(s))); }, in);

   CHECK(kept.size() >= std::size_t(cycles - 2));   // about one per cycle
   CHECK(kept.size() <= std::size_t(cycles + 1));
   for (auto const& p : kept)
      CHECK(p.amp > 0.0f);
}

TEST_CASE("peak_picker: silence and DC produce no picks")
{
   std::vector<float> zero(4800, 0.0f);
   q::peak_picker pk1;
   CHECK(run([&](float s){ return pk1(s); }, zero).empty());

   std::vector<float> dc(4800, 0.5f);
   q::peak_picker pk2;
   CHECK(run([&](float s){ return pk2(s); }, dc).empty());
}

///////////////////////////////////////////////////////////////////////////////
// Tier 2 -- real audio, with the signal conditioner in the chain. Emits a
// per-sample CSV for the viewer (results/peak_picker/<name>.csv) and a
// windowed-level golden for regression detection. Uses the bare core -- every
// local maximum during the note -- so we can see exactly what it picks before
// qualifiers are added.
///////////////////////////////////////////////////////////////////////////////

void process_audio(std::string const& name, q::frequency base)
{
   q::wav_reader src{"audio_files/" + name + ".wav"};
   REQUIRE(bool(src));
   float const sps = src.sps();
   std::vector<float> in(src.length());
   src.read(in);

   auto sc_conf = q::signal_conditioner::config{};
   auto sig_cond = q::signal_conditioner{sc_conf, base, base * 4, sps};

   q::peak_picker pk;

   constexpr int n_channels = 2;      // smoothed, peak-pulse
   std::vector<float> out(in.size() * n_channels);

   std::filesystem::create_directories("results/peak_picker");
   std::ofstream csv("results/peak_picker/" + name + ".csv");
   csv << "time, signal, smoothed, peak, amp, gate\n";
   csv << std::fixed << std::setprecision(6);

   for (std::size_t i = 0; i != in.size(); ++i)
   {
      float const raw = in[i];
      sig_cond(raw);
      bool const gate = sig_cond.gate();
      // The picker reads the conditioner's smoothed() tap: cleaned, but not
      // clipped or compressed, so crest apexes keep their true shape and
      // timing. It advances every sample (its state stays continuous), but a
      // pick counts only while the conditioner reports signal present --
      // gating is the caller's job, not the picker's.
      float const s = sig_cond.smoothed();
      auto const r = pk(s);
      bool const hit = r.hit && gate;

      auto const pos = i * n_channels;
      out[pos]     = s;
      out[pos + 1] = hit ? 0.8f : 0.0f;

      csv << double(i) / sps << ", " << raw << ", " << s << ", "
          << (hit ? 0.8f : 0.0f) << ", " << r.info.amp << ", "
          << int(gate) << '\n';
   }

   if (!q_test::suppress_wav())
   {
      q::wav_writer wav{"results/peak_picker/" + name + ".wav", n_channels, sps};
      wav.write(out);
   }

   std::filesystem::create_directories("results/golden");
   auto g_rows = q_test::windowed_level_csv(out, n_channels, sps);
   auto g_cols = q_test::level_columns(n_channels);
   q_test::write_golden_csv(
      "results/golden/peak_picker/" + name + ".csv", g_cols, g_rows);
   q_test::compare_golden_csv("peak_picker/" + name, g_cols, g_rows);
}

TEST_CASE("peak_picker: real guitar audio, picking the conditioner's smoothed tap")
{
   process_audio("1a-Low-E",           82.41_Hz);
   process_audio("2a-A",              110.00_Hz);
   process_audio("4a-G",              196.00_Hz);
   process_audio("6a-High-E",         329.63_Hz);
   process_audio("Tapping D",         146.83_Hz);
   process_audio("Hammer-Pull High E", 329.63_Hz);
   process_audio("GStaccato",         196.00_Hz);   // decaying-note tracking
}
