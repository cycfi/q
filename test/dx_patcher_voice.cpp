/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/synth/fm/dx_patcher.hpp>
#include <cmath>
#include <vector>

namespace q = cycfi::q;
using patcher = q::dx_patcher;
using voice = q::fm_voice;

static_assert(q::concepts::Patcher<patcher, q::fm_note>);

namespace
{
   constexpr float sps = 48000.0f;
   constexpr std::size_t n = 48000;

   // A one-operator voice: algorithm 32 with OP2..OP6 silent, instant
   // envelope, no LFO, no pitch envelope.
   patcher::config single()
   {
      patcher::config cfg;
      cfg.algorithm = {32, 0};
      for (int i = 1; i != 6; ++i)
         cfg.op[i].output_level = 0;
      return cfg;
   }

   // The frequency of a MIDI key, for the master phase iterator
   q::frequency key_hz(int key)
   {
      return q::frequency{440.0 * std::exp2((key - 69) / 12.0)};
   }

   // Render count samples with the master at the key's frequency
   std::vector<float> render(voice& v, int key, std::size_t count = n)
   {
      q::phase_iterator master{key_hz(key), sps};
      std::vector<float> y(count);
      for (auto& s : y)
         s = v(master++);
      return y;
   }

   double mag(std::vector<float> const& x, double f)
   {
      double re = 0, im = 0;
      for (std::size_t i = 0; i != x.size(); ++i)
      {
         double a = 2.0 * q::pi * f * i / sps;
         re += x[i] * std::cos(a);
         im += x[i] * std::sin(a);
      }
      return 2.0 * std::hypot(re, im) / x.size();
   }

   double rms(std::vector<float> const& x)
   {
      double s = 0;
      for (auto v : x)
         s += double(v) * v;
      return std::sqrt(s / x.size());
   }

   // Rising zero crossings per second
   double freq(std::vector<float> const& x)
   {
      int count = 0;
      for (std::size_t i = 1; i != x.size(); ++i)
         if (x[i-1] <= 0.0f && x[i] > 0.0f)
            ++count;
      return count * sps / x.size();
   }
}

TEST_CASE("A key plays its frequency, and transpose shifts it")
{
   auto cfg = single();
   patcher p_v{cfg, sps};
   voice v{p_v};
   v.attack(p_v, 69, 1.0f);                     // A4
   CHECK(freq(render(v, 69)) == Approx(440).margin(1));

   cfg.transpose = 12;                 // an octave down
   patcher p_w{cfg, sps};
   voice w{p_w};
   w.attack(p_w, 69, 1.0f);
   CHECK(freq(render(w, 69)) == Approx(220).margin(1));

   cfg.transpose = 31;                 // a fifth up
   patcher p_u{cfg, sps};
   voice u{p_u};
   u.attack(p_u, 69, 1.0f);
   CHECK(freq(render(u, 69)) == Approx(659.3).margin(2));
}

TEST_CASE("The voice is active until every envelope has released")
{
   auto cfg = single();
   cfg.op[0].env.rate[3] = 50;
   patcher p_v{cfg, sps};
   voice v{p_v};
   CHECK(!v.active());
   v.attack(p_v, 60, 1.0f);
   CHECK(v.active());
   render(v, 60, 4800);
   v.release();
   CHECK(v.active());
   render(v, 60, std::size_t(sps * 2));
   CHECK(!v.active());
   CHECK(render(v, 60, 1)[0] == 0.0f);
}

TEST_CASE("Velocity cuts the level by the sensitivity")
{
   auto cfg = single();
   auto level = [&](std::uint8_t sens, float velocity)
   {
      cfg.op[0].velocity_sens = sens;
      patcher p_v{cfg, sps};
   voice v{p_v};
      v.attack(p_v, 69, velocity);
      return mag(render(v, 69), 440);
   };
   CHECK(level(0, 0.2f) == Approx(1.0).margin(0.01));
   CHECK(level(7, 100 / 127.0f) == Approx(1.0).margin(0.06));   // 0 dB
   CHECK(level(7, 1.0f) == Approx(1.83).margin(0.05));         // +5.3 dB
   CHECK(level(7, 64 / 127.0f) == Approx(0.297).margin(0.02)); // -10.5 dB
   CHECK(level(7, 0.5f) < level(3, 0.5f));
   CHECK(level(3, 0.5f) < level(3, 0.8f));
   CHECK(level(3, 0.8f) < level(7, 1.0f));
}

TEST_CASE("Keyboard level scaling cuts or boosts away from the break point")
{
   auto cfg = single();
   cfg.op[0].output_level = 80;
   cfg.op[0].break_point = 39;         // C3, MIDI 60
   cfg.op[0].left_depth = 50;
   cfg.op[0].right_depth = 50;

   auto level = [&](std::uint8_t lc, std::uint8_t rc, std::uint8_t key)
   {
      cfg.op[0].left_curve = lc;
      cfg.op[0].right_curve = rc;
      patcher p_v{cfg, sps};
   voice v{p_v};
      v.attack(p_v, key, 1.0f);
      auto y = render(v, key);
      return mag(y, freq(y));
   };
   auto at_bp = level(patcher::neg_lin, patcher::neg_lin, 60);

   // Negative curves cut away from the break point, positive ones boost
   CHECK(level(patcher::neg_lin, patcher::neg_lin, 84) < at_bp);
   CHECK(level(patcher::neg_lin, patcher::neg_lin, 36) < at_bp);
   CHECK(level(patcher::pos_lin, patcher::pos_lin, 84) > at_bp);
   CHECK(level(patcher::pos_lin, patcher::pos_lin, 36) > at_bp);

   // An exponential curve does less near the break point than a linear one
   CHECK(level(patcher::neg_exp, patcher::neg_exp, 72)
      > level(patcher::neg_lin, patcher::neg_lin, 72));

   // Each side has its own curve
   CHECK(level(patcher::pos_lin, patcher::neg_lin, 36) > at_bp);
   CHECK(level(patcher::pos_lin, patcher::neg_lin, 84) < at_bp);
}

TEST_CASE("Rate scaling speeds the envelope up for higher keys")
{
   auto cfg = single();
   cfg.op[0].env.rate[0] = 50;
   cfg.op[0].rate_scaling = 7;

   auto rise_time = [&](std::uint8_t key)
   {
      patcher p_v{cfg, sps};
   voice v{p_v};
      v.attack(p_v, key, 1.0f);
      auto y = render(v, key, std::size_t(sps * 4));
      std::size_t i = 0;
      while (i != y.size() && std::abs(y[i]) < 0.9f)
         ++i;
      return i;
   };
   CHECK(rise_time(84) < rise_time(60));
   CHECK(rise_time(60) < rise_time(36));

   // Never slower than programmed: notes up to 26 keep their rates
   auto scaled = rise_time(26);
   cfg.op[0].rate_scaling = 0;
   CHECK(scaled == rise_time(26));
}

TEST_CASE("A quieter operator reaches its peak sooner")
{
   auto cfg = single();
   cfg.op[0].env.rate[0] = 40;
   auto peak_time = [&](std::uint8_t level)
   {
      cfg.op[0].output_level = level;
      patcher p_v{cfg, sps};
   voice v{p_v};
      v.attack(p_v, 60, 1.0f);
      auto y = render(v, 60, std::size_t(sps * 2));
      auto top = mag(std::vector<float>(y.end() - 4800, y.end()), freq(y));
      std::size_t i = 0;
      while (i != y.size() && std::abs(y[i]) < 0.9f * top)
         ++i;
      return i;
   };
   CHECK(peak_time(60) < peak_time(99) / 3);
}

TEST_CASE("The LFO adds vibrato by the pitch sensitivity")
{
   auto cfg = single();
   cfg.lfo.speed = 35;
   cfg.pitch_mod_depth = 99;
   cfg.pitch_mod_sens = 7;             // +/- 12 semitones

   patcher p_v{cfg, sps};
   voice v{p_v};
   v.attack(p_v, 69, 1.0f);
   auto y = render(v, 69);

   // With a full octave of vibrato the energy leaves the 440 Hz bin
   CHECK(mag(y, 440) < 0.5);
   CHECK(rms(y) == Approx(0.707).margin(0.02));

   cfg.pitch_mod_sens = 0;
   patcher p_w{cfg, sps};
   voice w{p_w};
   w.attack(p_w, 69, 1.0f);
   CHECK(mag(render(w, 69), 440) == Approx(1.0).margin(0.01));
}

TEST_CASE("The LFO adds tremolo by the amplitude sensitivity")
{
   auto cfg = single();
   cfg.lfo.speed = 35;
   cfg.amp_mod_depth = 99;
   cfg.op[0].amp_mod_sens = 3;

   patcher p_v{cfg, sps};
   voice v{p_v};
   v.attack(p_v, 69, 1.0f);
   auto y = render(v, 69);
   float lo = 1.0f;
   for (std::size_t i = 100; i < y.size(); i += 100)
   {
      float peak = 0.0f;
      for (std::size_t k = i - 100; k != i; ++k)
         peak = std::max(peak, std::abs(y[k]));
      lo = std::min(lo, peak);
   }
   CHECK(lo < 0.3f);
}

TEST_CASE("The pitch envelope bends around level 50")
{
   auto cfg = single();
   // Level 62 is 4.5 semitones up on the measured curve: 440 to 571 Hz
   cfg.pitch_env = {{99, 99, 99, 99}, {62, 62, 62, 50}};
   patcher p_v{cfg, sps};
   voice v{p_v};
   v.attack(p_v, 69, 1.0f);
   CHECK(freq(render(v, 69)) == Approx(571).margin(3));

   // At rest the pitch envelope sits at L4: no bend before the first note
   cfg.pitch_env = {{99, 99, 99, 99}, {50, 50, 50, 50}};
   patcher p_w{cfg, sps};
   voice w{p_w};
   w.attack(p_w, 69, 1.0f);
   render(w, 69, 300);                     // past the amplitude attack
   auto y = render(w, 69, 200);
   for (std::size_t i = 0; i != 200; ++i)
      REQUIRE(y[i] == Approx(std::sin(2.0 * q::pi * 440.0 * (i + 300) / sps))
         .margin(0.01));
}

TEST_CASE("Oscillator key sync makes a note start the same way twice")
{
   auto cfg = single();
   cfg.op[0].env.rate[3] = 99;
   patcher p_v{cfg, sps};
   voice v{p_v};
   v.attack(p_v, 69, 1.0f);
   auto a = render(v, 69, 1000);
   v.release();
   render(v, 69, 1000);
   v.attack(p_v, 69, 1.0f);
   auto b = render(v, 69, 1000);
   for (std::size_t i = 0; i != 1000; ++i)
      REQUIRE(a[i] == b[i]);
}

TEST_CASE("An even operator envelope level acts as the odd one above it")
{
   auto cfg = single();
   cfg.op[0].env = {{99, 99, 99, 99}, {80, 80, 80, 0}};
   patcher p_a{cfg, sps};
   voice a{p_a};
   cfg.op[0].env = {{99, 99, 99, 99}, {81, 81, 81, 0}};
   patcher p_b{cfg, sps};
   voice b{p_b};
   a.attack(p_a, 69, 1.0f);
   b.attack(p_b, 69, 1.0f);
   auto ya = render(a, 69);
   auto yb = render(b, 69);
   CHECK(mag(ya, 440) == Approx(mag(yb, 440)).epsilon(1e-4));
   CHECK(mag(ya, 440) == Approx(q::detail::dx_level_gain(81)).epsilon(0.01));

   // The pitch envelope's levels are not: 50 stays no shift
   cfg.op[0].env = {{99, 99, 99, 99}, {99, 99, 99, 0}};
   cfg.pitch_env = {{99, 99, 99, 99}, {50, 50, 50, 50}};
   patcher p_c{cfg, sps};
   voice c{p_c};
   c.attack(p_c, 69, 1.0f);
   CHECK(freq(render(c, 69)) == Approx(440).margin(1));
}

TEST_CASE("Transpose moves the key the scalings see")
{
   auto cfg = single();
   cfg.op[0].output_level = 80;
   cfg.op[0].break_point = 39;         // C3
   cfg.op[0].left_depth = 50;
   cfg.op[0].right_depth = 50;
   cfg.op[0].rate_scaling = 7;
   cfg.op[0].env.rate[0] = 50;

   auto probe = [&](std::uint8_t transpose, std::uint8_t key)
   {
      cfg.transpose = transpose;
      patcher p_v{cfg, sps};
   voice v{p_v};
      v.attack(p_v, key, 1.0f);
      auto y = render(v, key, std::size_t(sps * 2));
      std::size_t i = 0;
      while (i != y.size() && std::abs(y[i]) < 0.5f * mag(y, freq(y)))
         ++i;
      return std::pair{mag(y, freq(y)), i};
   };
   auto [level_down, rise_down] = probe(12, 72);    // sounds as key 60
   auto [level_at, rise_at] = probe(24, 60);
   CHECK(level_down == Approx(level_at).epsilon(0.02));
   CHECK(rise_down == rise_at);
}

TEST_CASE("Copies of a voice play alike from the same patcher")
{
   auto cfg = single();
   cfg.op[0].env.rate[3] = 60;
   patcher p_a{cfg, sps};
   voice a{p_a};
   voice b = a;
   a.attack(p_a, 64, 0.8f);
   b.attack(p_a, 64, 0.8f);
   q::phase_iterator master{key_hz(64), sps};
   for (int i = 0; i != 4800; ++i, ++master)
      REQUIRE(a(master) == b(master));
}
