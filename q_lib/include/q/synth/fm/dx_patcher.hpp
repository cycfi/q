/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DX_PATCHER_HPP_SEPTEMBER_15_2026)
#define CYCFI_Q_DX_PATCHER_HPP_SEPTEMBER_15_2026

#include <q/synth/fm/fm_voice.hpp>
#include <q/synth/fm/dx7_routing.hpp>
#include <q/synth/fm/detail/dx_level.hpp>
#include <q/synth/fm/detail/dx_tables.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // The DX7 layer. A patch holds its parameters in the 0..99 style ranges
   // a cartridge stores, so a preset maps one to one: dx_patch below, of
   // dx_op_config operators. dx_patcher compiles one, for a sample rate,
   // into what an fm_voice takes: the voice's config, and per operator the
   // frequency ratio and the envelope's rates as times and levels in dB,
   // for every key and velocity, so a note-on is table reads. The
   // DX7's laws (the notes after dx_patcher) and its measured tables
   // (detail/) live here and nowhere else.
   //
   // A Patcher (see fm_voice): fm_voice{patcher} sets a voice up, and
   // voice.attack(patcher, key, velocity) plays a key.
   ////////////////////////////////////////////////////////////////////////////
   enum dx_curve : std::uint8_t
   {
      dx_neg_lin, dx_neg_exp, dx_pos_exp, dx_pos_lin
   };

   struct dx_eg_config
   {
      std::uint8_t   rate[4]  = {99, 99, 99, 99};   // R1..R4
      std::uint8_t   level[4] = {99, 99, 99, 0};    // L1..L4
   };

   struct dx_op_config
   {
      std::uint8_t   output_level  = 99;    // 0..99, note 1
      bool           fixed         = false; // fixed frequency, note 5
      std::uint8_t   coarse        = 1;     // frequency coarse 0..31
      std::uint8_t   fine          = 0;     // frequency fine 0..99
      std::uint8_t   detune        = 7;     // 0..14, 7 is none, note 6
      dx_eg_config   env;                   // note 2
      std::uint8_t   break_point   = 39;    // 0..99, 39 is C3, note 7
      std::uint8_t   left_depth    = 0;     // 0..99
      std::uint8_t   right_depth   = 0;
      std::uint8_t   left_curve    = dx_neg_lin;
      std::uint8_t   right_curve   = dx_neg_lin;
      std::uint8_t   rate_scaling  = 0;     // 0..7, note 8
      std::uint8_t   amp_mod_sens  = 0;     // 0..3, note 10
      std::uint8_t   velocity_sens = 0;     // 0..7, note 9
   };

   struct dx_algorithm_config
   {
      std::uint8_t   algorithm = 1;         // 1..32, note 3
      std::uint8_t   feedback  = 0;         // 0..7, note 4
   };

   struct dx_lfo_config
   {
      std::uint8_t   speed     = 35;        // 0..99, note 12
      std::uint8_t   delay     = 0;         // 0..99
      bool           key_sync  = true;
      std::uint8_t   wave      = lfo_gen::triangle;
   };

   struct dx_patch
   {
      using eg_config = dx_eg_config;

      dx_op_config   op[6];                 // OP1..OP6
      eg_config      pitch_env = {{99, 99, 99, 99}, {50, 50, 50, 50}};
      dx_algorithm_config algorithm;
      dx_lfo_config  lfo;
      std::uint8_t   pitch_mod_depth = 0;   // LFO PMD 0..99, note 10
      std::uint8_t   amp_mod_depth = 0;     // LFO AMD 0..99
      std::uint8_t   pitch_mod_sens = 3;    // 0..7, note 10
      bool           osc_key_sync = true;
      std::uint8_t   transpose = 24;        // 0..48, 24 is C3, note 13
      char           name[11] = "INIT VOICE";
   };

   // One compiled operator
   struct dx_op_patch
   {
      double         ratio[128];            // to the key's, note 5
      phase          fixed_step;            // fixed mode
      std::uint8_t   qrate[4];              // envelope rates, note 2
      std::uint8_t   qrate_offset[128];     // rate scaling per key
      float          output_db[128];        // output level per key
      float          velocity_db[128];      // dB per velocity
      float          eg_db[4];              // envelope levels
   };

   struct dx_patcher
   {
      using curve = dx_curve;
      using config = dx_patch;
      using op_config = dx_op_config;
      using op_patch = dx_op_patch;

      static constexpr auto neg_lin = dx_neg_lin;
      static constexpr auto neg_exp = dx_neg_exp;
      static constexpr auto pos_exp = dx_pos_exp;
      static constexpr auto pos_lin = dx_pos_lin;

                     dx_patcher(config const& cfg, float sps);

      // Everything a note-on needs for a key and a velocity 0..1
      void           note(
                        std::uint8_t key, float velocity, fm_note& out
                     ) const;

      config         cfg;
      float          sps;
      fm_voice_config voice;
      op_patch       op[6];
      double         rate_time[64];         // seconds, per qrate, note 2

   private:

      void           compile(std::size_t op);
   };

   // The DX7's laws are public work by others, no code of theirs used:
   // the levels, rates and envelope shape are the DX7s measurements on
   // the Music Synthesizer for Android wiki (Raph Levien and
   // contributors), and Ken Shirriff's reverse engineering of the chip
   // (righto.com, 2021) is the account of the hardware behind them. The
   // rest is measured on a reference emulation as a black box. The KB
   // (q/fm-synthesis) has the sources and the measurements.
   //
   // Notes:
   //
   // 1. Level: an operator's total level is counted in steps of 1/256 of a
   //    doubling (0.0235 dB), 64 per envelope level unit and 32 per output
   //    level unit, full when both are 99 (detail/dx_level.hpp). Output
   //    levels move in 0.75 dB steps, steeper below 20; envelope levels
   //    above 19 move in pairs. So an envelope's dB levels are the sum of
   //    its own, the output level's, scaling's and velocity's.
   // 2. Rate: qrate = rate * 41 / 64 (0..63). A decay falls by
   //    2^(qrate / 4) * (1 + (qrate mod 4) / 4) counts per 4096 clocks of
   //    the DX7's 49096 Hz clock: 0.28 dB/s at qrate 0, doubling every 4
   //    qrates. (The reference emulation runs a 44100 Hz clock, so its
   //    envelopes are 11% slower.)
   // 3. The 32 algorithms are the DX7 chart: dx7_routing (its own header).
   // 4. Feedback level 0..7 halves per step below 7. At 7, a full level
   //    carrier modulates itself by max_index / 8, about 1.6 radians, the
   //    DX7's saw-like feedback tone; a modulator's loop runs 4 times as
   //    strong. max_index, the deviation of a full level modulator, is 4
   //    pi, consistent with Chowning and Bristow's table (about 13).
   // 5. Ratio mode: coarse 0 is 0.5, else coarse, times (1 + fine/100),
   //    times the detune; the ratio table scales the key's frequency,
   //    with the transpose folded in. Fixed mode: 10^(coarse mod 4) *
   //    10^(fine/100) Hz, 1 Hz to 9772 Hz (DX7 manual), with ratios of 0.
   // 6. Detune: steps / 7 * (52.28 f^-0.274 - 2.94) cents, fitted to +/-7
   //    measured at 27.5 Hz to 3.5 kHz (18 to 2.8 cents), within 0.25
   //    cents; neither constant cents nor constant Hz.
   // 7. Keyboard level scaling works in groups of 3 notes starting 5 below
   //    the break point (key 21 + break_point), on the internal output
   //    level (0..127, clamped). Linear curves move it 0.0791 units per
   //    depth unit per group; exponential ones by dx_ex_scaling at depth
   //    99, scaled by depth / 99. Negative curves cut, positive ones boost.
   // 8. Keyboard rate scaling: the qrate goes up by depth * clamp(note / 3
   //    - 7, 0, 31) / 8, rounded down.
   // 9. Velocity: dx_velocity_db at sensitivity 7, scaled by sens / 7.
   // 10. Pitch modulation: dx_pm_sens semitones at full depth, linear in
   //    the depth. Amplitude modulation cuts A (e^(k depth) - 1) dB at the
   //    LFO's trough, A and k by sensitivity (dx_am_sens).
   // 11. The pitch envelope ramps linearly in semitones between its levels
   //    (dx_pitch_eg_32nds, in 1/32 octave), at dx_pitch_rate semitones
   //    per second.
   // 12. LFO speed to Hz: dx_lfo_hz, measured at every speed. Delay: the
   //    LFO waits, then fades in linearly; the fade is centred on 0.1541
   //    (e^(0.03008 delay) - 1) s and lasts that long, at most 0.75 s.
   // 13. Transpose moves the key the scalings see as well as the pitch.

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      constexpr float dx_max_index = float(4 * pi);       // note 4
      constexpr float dx_envelope_clock = 49096.0f;       // note 2


      inline double dx_key_hz(int note)
      {
         return 440.0 * std::exp2((note - 69) / 12.0);
      }

      inline double dx_frequency_ratio(dx_op_config const& op)
      {
         double coarse = op.coarse == 0 ? 0.5 : op.coarse;
         return coarse * (1.0 + op.fine / 100.0);
      }

      inline double dx_fixed_frequency(dx_op_config const& op)
      {
         return std::pow(10.0, (op.coarse % 4) + op.fine / 100.0);
      }

      // Detune in cents at a frequency (note 6)
      inline double dx_detune_cents(int steps, double hz)
      {
         return steps / 7.0
            * (52.275653 * std::pow(hz, -0.273999) - 2.936387);
      }

      // Decay speed in counts per second at a qrate (note 2)
      inline float dx_counts_per_second(int qrate)
      {
         return dx_envelope_clock / 4096.0f * float(1 << (qrate >> 2))
            * (1.0f + (qrate & 3) / 4.0f);
      }

      inline int dx_floor_div3(int x)
      {
         return x >= 0 ? x / 3 : -((2 - x) / 3);
      }

      // dB by velocity 1..127 at sensitivity 7 (note 9)
      inline float dx_velocity_gain_db(int velocity)
      {
         auto p = std::clamp(velocity - 1, 0, 126) / 3.0f;
         auto i = std::min(int(p), 41);
         auto f = p - i;
         return dx_velocity_db[i]
            + (dx_velocity_db[i + 1] - dx_velocity_db[i]) * f;
      }

      // Exponential scaling units at depth 99 by group distance (note 7)
      inline float dx_ex_units(int distance)
      {
         if (distance < 18)
            return dx_ex_scaling[distance];
         return (distance - 1)
            + 4.1851f * (std::exp(0.2278f * (distance - 9)) - 1);
      }

      // Output level change in internal units for a note (note 7)
      inline float dx_scale_units(
         dx_op_config const& op, int note)
      {
         int group = dx_floor_div3(note - (op.break_point + 21 - 5));
         if (group == 0)
            return 0.0f;

         bool left = group < 0;
         int distance = std::abs(group);
         float depth = left ? op.left_depth : op.right_depth;
         auto c = left ? op.left_curve : op.right_curve;
         bool lin = c == dx_neg_lin || c == dx_pos_lin;
         float units = lin ?
            depth * distance * 0.0791f
            : depth / 99.0f * dx_ex_units(distance);
         bool cut = c == dx_neg_lin || c == dx_neg_exp;
         return cut ? -units : units;
      }
   }

   inline dx_patcher::dx_patcher(config const& cfg_, float sps_)
    : cfg{cfg_}
    , sps{sps_}
   {
      for (int q = 0; q != 64; ++q)
         rate_time[q] = dx_envelope_gen::range.rep
            / (detail::dx_counts_per_second(q) * detail::dx_db_per_count);
      for (std::size_t i = 0; i != 6; ++i)
         compile(i);

      auto n = std::clamp<int>(cfg.algorithm.algorithm, 1, 32);
      auto& alg = voice.algorithm;
      alg.route = dx7_routing[n - 1];                       // note 3
      alg.index = detail::dx_max_index;
      auto level = std::min<int>(cfg.algorithm.feedback, 7);   // note 4
      alg.feedback = level == 0 ? 0.0f :
         detail::dx_max_index * float(1 << level) / 1024.0f;
      auto source = fm_routing::mask(1 << alg.route.get().fb_src);
      if (!(alg.route.carriers() & source))
         alg.feedback *= 4.0f;

      auto& lfo = voice.lfo;                                // note 12
      lfo.rate = detail::dx_lfo_hz[std::min<int>(cfg.lfo.speed, 99)];
      auto centre = 0.15413f * (std::exp(0.03008f * cfg.lfo.delay) - 1.0f);
      lfo.fade = std::min(0.75f, centre);
      lfo.delay = centre - lfo.fade / 2;
      lfo.key_sync = cfg.lfo.key_sync;
      lfo.wave = cfg.lfo.wave;

      for (std::size_t s = 0; s != 4; ++s)                 // note 11
      {
         auto level = std::min<int>(cfg.pitch_env.level[s], 99);
         auto rate = std::min<int>(cfg.pitch_env.rate[s], 99);
         voice.pitch_env_level[s] =
            detail::dx_pitch_eg_32nds[level] * 0.375f;
         voice.pitch_env_rate[s] = detail::dx_pitch_rate[rate];
      }
      voice.pitch_mod = detail::dx_pm_sens[cfg.pitch_mod_sens & 7]
         * cfg.pitch_mod_depth / 99.0f;                    // note 10
      for (std::size_t i = 0; i != 6; ++i)
      {
         auto ams = cfg.op[i].amp_mod_sens & 3;
         voice.amp_mod[i] = detail::dx_am_sens[ams].a
            * (std::exp(detail::dx_am_sens[ams].k * cfg.amp_mod_depth) - 1);
      }
      voice.key_sync = cfg.osc_key_sync;
   }

   inline void dx_patcher::compile(std::size_t i)
   {
      auto const& src = cfg.op[i];
      auto& dst = op[i];
      int steps = int(src.detune) - 7;
      auto detuned = [steps](double hz)
      {
         return steps ?
            hz * std::exp2(detail::dx_detune_cents(steps, hz) / 1200.0) : hz;
      };
      dst.fixed_step = src.fixed ?                          // note 5
         phase{frequency{detuned(detail::dx_fixed_frequency(src))}, sps}
         : phase{};

      for (int key = 0; key != 128; ++key)
      {
         int note = key + int(cfg.transpose) - 24;       // note 13
         auto hz = detail::dx_key_hz(note) * detail::dx_frequency_ratio(src);
         dst.ratio[key] = src.fixed ?
            0.0 : detuned(hz) / detail::dx_key_hz(key);

         int group = std::clamp(detail::dx_floor_div3(note) - 7, 0, 31);
         dst.qrate_offset[key] = (src.rate_scaling * group) >> 3;  // note 8

         auto units = src.output_level == 0 ? 0.0f :        // note 1
            std::clamp(detail::dx_output_level_units(src.output_level)
               + detail::dx_scale_units(src, note), 0.0f, 127.0f);
         dst.output_db[key] =
            32.0f * (units - 127) * detail::dx_db_per_count;
      }

      for (int v = 0; v != 128; ++v)                       // note 9
         dst.velocity_db[v] = detail::dx_velocity_gain_db(std::max(v, 1))
            * src.velocity_sens / 7.0f;

      for (std::size_t s = 0; s != 4; ++s)                 // note 2
      {
         dst.qrate[s] = (src.env.rate[s] * 41) >> 6;
         dst.eg_db[s] = 64.0f
            * (detail::dx_eg_level_units(src.env.level[s]) - 63)
            * detail::dx_db_per_count;
      }
   }

   inline void dx_patcher::note(
      std::uint8_t key, float velocity, fm_note& out) const
   {
      auto k = key & 127;
      auto v = std::clamp(int(velocity * 127.0f + 0.5f), 1, 127);
      for (std::size_t i = 0; i != 6; ++i)
      {
         auto const& o = op[i];
         auto& p = out.op[i];
         p.ratio = o.ratio[k];
         p.fixed_step = o.fixed_step;
         for (std::size_t s = 0; s != 4; ++s)
         {
            auto q = std::min(o.qrate[s] + o.qrate_offset[k], 63);
            p.env.rate[s] = duration{rate_time[q]};
            p.env.level[s] =
               dB(o.eg_db[s] + o.output_db[k] + o.velocity_db[v]);
         }
      }
   }
}

#endif
