/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/literals.hpp>
#include <q/synth/fm/dx_patcher.hpp>
#include <cmath>

namespace q = cycfi::q;
using namespace q::literals;
using patcher = q::dx_patcher;

namespace
{
   constexpr float sps = 48000.0f;

   float units(int level)
   {
      return float(q::detail::dx_output_level_units(level));
   }

   // The dB of an output level in internal units, the envelope at full
   float db_of(float units)
   {
      return 32.0f * (units - 127) * q::detail::dx_db_per_count;
   }
}

TEST_CASE("Level scaling moves in groups of 3 from 5 below the break point")
{
   patcher::config cfg;
   auto& op = cfg.op[0];
   op.output_level = 60;
   op.break_point = 39;                        // C3, MIDI 60
   op.left_depth = op.right_depth = 99;
   op.left_curve = op.right_curve = patcher::neg_lin;
   patcher p{cfg, sps};
   auto const& out = p.op[0].output_db;

   // Unscaled from 55 to 57
   for (int k : {55, 56, 57})
      CHECK(out[k] == Approx(db_of(units(60))));

   // One group either side, equal within the group
   auto one = db_of(units(60) - 99 * 0.0791f);
   for (int k : {52, 53, 54, 58, 59, 60})
      CHECK(out[k] == Approx(one));

   // Two groups up cuts twice as much
   CHECK(out[61] == Approx(db_of(units(60) - 2 * 99 * 0.0791f)));
}

TEST_CASE("Boosts clamp at full output, cuts at silence")
{
   patcher::config cfg;
   auto& op = cfg.op[0];
   op.output_level = 60;
   op.left_depth = op.right_depth = 99;
   op.left_curve = patcher::pos_lin;
   op.right_curve = patcher::neg_lin;
   patcher p{cfg, sps};
   CHECK(p.op[0].output_db[21] == 0.0f);
   CHECK(p.op[0].output_db[127] == Approx(db_of(0)));    // -95.6 dB
}

TEST_CASE("An exponential curve does little near the break point")
{
   patcher::config cfg;
   auto& op = cfg.op[0];
   op.output_level = 60;
   op.right_depth = 99;
   op.right_curve = patcher::neg_exp;
   patcher p{cfg, sps};
   CHECK(p.op[0].output_db[58] == Approx(db_of(units(60))));     // 1: 0
   CHECK(p.op[0].output_db[61] == Approx(db_of(units(60) - 1)));  // 2: 1
   CHECK(p.op[0].output_db[106] == Approx(db_of(units(60) - 38)));// 17: 38
}

TEST_CASE("Rate scaling offset by key, and transpose shifts the key")
{
   patcher::config cfg;
   cfg.op[0].rate_scaling = 7;
   patcher p{cfg, sps};
   CHECK(p.op[0].qrate_offset[60] == (7 * (20 - 7)) >> 3);
   CHECK(p.op[0].qrate_offset[24] == 0);

   cfg.transpose = 12;
   patcher t{cfg, sps};
   CHECK(t.op[0].qrate_offset[72] == p.op[0].qrate_offset[60]);
   CHECK(t.op[0].ratio[72] * q::detail::dx_key_hz(72)
      == Approx(p.op[0].ratio[60] * q::detail::dx_key_hz(60)));
}

TEST_CASE("Velocity adds its measured dB, scaled by sensitivity")
{
   patcher::config cfg;
   cfg.op[0].velocity_sens = 7;
   cfg.op[1].velocity_sens = 0;
   patcher p{cfg, sps};
   auto db = [&](int v) { return p.op[0].velocity_db[v]; };
   CHECK(db(127) == Approx(5.27f).margin(0.01));
   CHECK(std::abs(db(100)) < 0.5f);
   CHECK(db(64) == Approx(-10.54f).margin(0.01));
   CHECK(p.op[1].velocity_db[64] == 0.0f);
}

TEST_CASE("The ratio holds coarse and fine, and is the same at every key")
{
   auto ratio = [](std::uint8_t coarse, std::uint8_t fine = 0)
   {
      patcher::config cfg;
      cfg.op[0].coarse = coarse;
      cfg.op[0].fine = fine;
      patcher p{cfg, sps};
      CHECK(p.op[0].ratio[36] == p.op[0].ratio[96]);
      CHECK(p.op[0].fixed_step.rep == 0);
      return p.op[0].ratio[69];
   };
   CHECK(ratio(1) == Approx(1.0));
   CHECK(ratio(2) == Approx(2.0));
   CHECK(ratio(0) == Approx(0.5));
   CHECK(ratio(1, 50) == Approx(1.5));
   CHECK(ratio(3, 20) == Approx(3.6));
}

TEST_CASE("A fixed frequency operator has its step and no ratio")
{
   auto step = [](std::uint8_t coarse, std::uint8_t fine = 0)
   {
      patcher::config cfg;
      cfg.op[0].fixed = true;
      cfg.op[0].coarse = coarse;
      cfg.op[0].fine = fine;
      patcher p{cfg, sps};
      CHECK(p.op[0].ratio[69] == 0.0);
      return p.op[0].fixed_step.rep;
   };
   CHECK(step(0) == q::phase{1_Hz, sps}.rep);
   CHECK(step(2) == q::phase{100_Hz, sps}.rep);
   CHECK(step(6) == q::phase{100_Hz, sps}.rep);
   auto top = q::frequency{std::pow(10.0, 3.99)};
   CHECK(step(3, 99) == q::phase{top, sps}.rep);
}

TEST_CASE("Detune spreads the ratio by a few cents, more lower down")
{
   patcher::config cfg;
   cfg.op[0].detune = 14;
   cfg.op[1].detune = 0;
   patcher p{cfg, sps};

   auto cents = q::detail::dx_detune_cents(7, 440.0);
   CHECK(cents == Approx(6.80).margin(0.25));
   CHECK(p.op[0].ratio[69] == Approx(std::exp2(cents / 1200)));
   CHECK(p.op[1].ratio[69] == Approx(std::exp2(-cents / 1200)));
   CHECK(p.op[0].ratio[57] > p.op[0].ratio[69]);     // 110 Hz: more cents
}

TEST_CASE("Pitch envelope levels in semitones, depths by sensitivity")
{
   patcher::config cfg;
   cfg.pitch_env = {{99, 99, 99, 99}, {99, 0, 62, 50}};
   cfg.pitch_mod_sens = 7;
   cfg.pitch_mod_depth = 99;
   cfg.amp_mod_depth = 99;
   cfg.op[0].amp_mod_sens = 3;
   patcher p{cfg, sps};
   CHECK(p.voice.pitch_env_level[0] == Approx(47.625f));
   CHECK(p.voice.pitch_env_level[1] == Approx(-48.0f));
   CHECK(p.voice.pitch_env_level[2] == Approx(4.5f));
   CHECK(p.voice.pitch_env_level[3] == 0.0f);
   CHECK(p.voice.pitch_env_rate[0] == Approx(q::detail::dx_pitch_rate[99]));
   CHECK(p.voice.pitch_mod == Approx(11.914f));
   CHECK(p.voice.amp_mod[0] == Approx(90.0f).epsilon(0.02));
   CHECK(p.voice.amp_mod[1] == 0.0f);
}

TEST_CASE("Output level is 0.75 dB per step, steeper below 20")
{
   auto level = [](std::uint8_t l)
   {
      patcher::config cfg;
      cfg.op[0].output_level = l;
      return patcher{cfg, sps}.op[0].output_db[60];
   };
   CHECK(level(99) == 0.0f);
   CHECK(level(91) == Approx(-6.02f).margin(0.01));
   CHECK(level(59) == Approx(-30.1f).margin(0.02));
   CHECK(level(20) == Approx(-59.45f).margin(0.02));
   CHECK(level(15) < level(20));
   CHECK(level(10) < level(15));
   CHECK(level(0) < q::dx_envelope_gen::floor.rep);        // silent
}

TEST_CASE("A note-on gives each operator its ratio and envelope in dB")
{
   patcher::config cfg;
   cfg.op[0].env = {{40, 60, 50, 70}, {99, 60, 80, 0}};
   cfg.op[1].coarse = 3;
   cfg.op[1].output_level = 80;
   patcher p{cfg, sps};
   q::fm_note n;
   p.note(60, 1.0f, n);

   auto counts_per_second = [](int rate)
   {
      int qr = (rate * 41) >> 6;
      return 49096.0f / 4096 * float(1 << (qr >> 2)) * (1 + (qr & 3) / 4.0f);
   };
   // A rate is the time the envelope's whole range would take
   auto const& e = n.op[0].env;
   for (int s = 0; s != 4; ++s)
   {
      auto db_s = counts_per_second(cfg.op[0].env.rate[s])
         * q::detail::dx_db_per_count;
      CHECK(as_double(e.rate[s])
         == Approx(q::dx_envelope_gen::range.rep / db_s));
   }
   CHECK(e.level[0].rep == 0.0);
   CHECK(e.level[1].rep == Approx(64 * (q::detail::dx_eg_level_units(60) - 63)
      * q::detail::dx_db_per_count));
   CHECK(e.level[3].rep < q::dx_envelope_gen::floor.rep);
   CHECK(n.op[0].ratio == 1.0);
   CHECK(n.op[1].ratio == 3.0);
   CHECK(n.op[1].env.level[0] == Approx(db_of(units(80))));
}

TEST_CASE("The routing table is the DX7 chart")
{
   // Carrier counts from the chart, algorithms 1..32
   constexpr std::size_t carriers[32] =
   {
      2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
      1, 1, 3, 3, 4, 4, 4, 5, 5, 3, 3, 3, 4, 4, 5, 6
   };
   for (std::uint8_t a = 1; a <= 32; ++a)
   {
      patcher::config cfg;
      cfg.algorithm.algorithm = a;
      q::fm_algorithm alg{patcher{cfg, sps}.voice.algorithm};
      CHECK(alg.num_carriers() == carriers[a - 1]);
      CHECK(alg.is_carrier(0));               // OP1 always
   }
   // Algorithms 4 and 6 loop across operators
   CHECK(q::dx7_routing[3].get().fb_src == 3);
   CHECK(q::dx7_routing[3].get().fb_dst == 5);
   CHECK(q::dx7_routing[5].get().fb_src == 4);
}

TEST_CASE("Feedback 7: pi/2 radians for a carrier, 4 times a modulator")
{
   patcher::config cfg;
   cfg.algorithm = {32, 7};                   // OP6 into itself, a carrier
   CHECK(patcher{cfg, sps}.voice.algorithm.feedback
      == Approx(q::pi / 2).epsilon(1e-3));
   cfg.algorithm = {32, 6};
   CHECK(patcher{cfg, sps}.voice.algorithm.feedback
      == Approx(q::pi / 4).epsilon(1e-3));
   cfg.algorithm = {32, 0};
   CHECK(patcher{cfg, sps}.voice.algorithm.feedback == 0.0f);
   cfg.algorithm = {2, 7};                    // OP2, a modulator
   CHECK(patcher{cfg, sps}.voice.algorithm.feedback
      == Approx(2 * q::pi).epsilon(1e-3));
   CHECK(patcher{cfg, sps}.voice.algorithm.index == Approx(4 * q::pi));
}

TEST_CASE("LFO speed to Hz, delay to a wait and a fade")
{
   patcher::config cfg;
   cfg.lfo.speed = 35;
   cfg.lfo.delay = 99;
   auto const& lfo = patcher{cfg, sps}.voice.lfo;
   CHECK(lfo.rate == Approx(q::detail::dx_lfo_hz[35]));
   auto centre = 0.15413f * (std::exp(0.03008f * 99) - 1);   // 2.95 s
   CHECK(lfo.fade == 0.75f);
   CHECK(lfo.delay == Approx(centre - 0.375f));

   cfg.lfo.delay = 0;
   CHECK(patcher{cfg, sps}.voice.lfo.delay == 0.0f);
   CHECK(patcher{cfg, sps}.voice.lfo.fade == 0.0f);
}
