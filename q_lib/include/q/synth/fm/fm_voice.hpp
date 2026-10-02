/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FM_VOICE_HPP_SEPTEMBER_15_2026)
#define CYCFI_Q_FM_VOICE_HPP_SEPTEMBER_15_2026

#include <q/support/decibel.hpp>
#include <q/synth/fm/fm_operator.hpp>
#include <q/synth/fm/fm_algorithm.hpp>
#include <q/synth/gen/lfo_gen.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cycfi::q
{
   namespace detail
   {
      // The pitch envelope: a ramp in semitones between its levels
      struct fm_pitch_env
      {
         void           set(
                           std::array<float, 4> const& semis
                         , std::array<float, 4> const& rate   // per second
                         , float sps
                        );

         // New levels and rates, the ramp carrying on from where it is
         void           retarget(
                           std::array<float, 4> const& semis
                         , std::array<float, 4> const& rate
                         , float sps
                        );
         void           attack()       { _i = 0; }
         void           release()      { _i = 3; }
         float          operator()();

      private:

         std::array<float, 4> _target;
         std::array<float, 4> _speed;     // semitones per sample
         float          _semis = 0.0f;
         int            _i = 4;           // 0..2 attack, 3 release, 4 hold
      };
   }

   // What a voice is set up with once. A voice reads the first entries
   // of anything per operator, up to fm_max_operators.
   struct fm_voice_config
   {
      fm_algorithm::config  algorithm;
      lfo_gen::config       lfo;
      float                 pitch_mod = 0.0f;  // semitones at the LFO's peak

      // a cut in decibels at the LFO's trough, one per operator
      float                 amp_mod[fm_max_operators] = {};

      // The depths the mod wheel reaches at full: it takes each from
      // the one above toward these, whichever is the deeper.
      float                 pitch_mod_wheel = 0.0f;   // semitones
      float                 amp_mod_wheel[fm_max_operators] = {};  // dB

      std::array<float, 4>  pitch_env_level = {};  // semitones
      std::array<float, 4>  pitch_env_rate = {};   // semitones per second
      bool                  key_sync = true;   // phases restart at note-on
   };

   // What a note-on brings: per operator, the pitch (a ratio of the
   // master's, or 0 with a fixed step) and the envelope, for the
   // operators' envelope type; fm_note is for the DX7 shaped one
   template <typename Env>
   struct basic_fm_note
   {
      // Every operator of the voice needs its own: all of them sound
      // unless the routing has them modulating something
      struct op_params
      {
         double         ratio = 1.0;
         phase          fixed_step = {};
         typename Env::config env;
      };

      op_params      op[fm_max_operators];
   };

   using fm_note = basic_fm_note<dx_envelope_gen>;

   namespace concepts
   {
      // A patcher: compiles a patch into what a voice takes: the voice's
      // config at its sample rate, and a note for a key and a velocity
      // (0..1), so a patch can vary by both. dx_patcher is one.
      template <typename T, typename Note>
      concept Patcher =
         requires(T const& p, std::uint8_t key, float velocity, Note& n)
         {
            { p.voice } -> std::convertible_to<fm_voice_config>;
            { p.sps } -> std::convertible_to<float>;
            p.note(key, velocity, n);
         };
   }

   ////////////////////////////////////////////////////////////////////////////
   // basic_fm_voice: one sounding note of any number of operators (up to
   // fm_max_operators), each its own basic_fm_operator type, through an
   // algorithm, with an LFO for vibrato and tremolo and a pitch envelope;
   // fm_voice is the one with six fm_operators (sine, DX7 shaped
   // envelope). The pitch is the caller's,
   // a master phase iterator passed per sample: its step is the note, from
   // a keyboard, a pitch detector, or anything else. The voice bends it by
   // the LFO and the pitch envelope and hands it to the operators, each of
   // which follows it by its ratio (a fixed frequency operator ignores
   // it). A note-on brings the operators' ratios and envelopes, so a patch
   // can vary them by key and velocity: a Patcher does that for a key.
   //
   // Two inputs a player moves while the note sounds: the mod wheel, 0 to
   // 1, deepening the LFO's vibrato and tremolo toward the config's wheel
   // depths, and a switch per operator, a bit each in enable's mask. A
   // switched-off operator neither sounds nor modulates.
   //
   // update gives a sounding note a new patch, as a DX7's notes take a
   // voice change or an edit: each part takes its new settings and goes
   // on from where it is. Envelopes, pitch envelope and phases carry on,
   // so a released note held at a level fades if the new one is lower.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Op, typename... RestOps>
   struct basic_fm_voice
   {
      static constexpr std::size_t size = 1 + sizeof...(RestOps);
      static_assert(size <= fm_max_operators, "too many operators");

      using envelope_type = typename Op::envelope_type;
      static_assert(
         (std::is_same_v<
            typename RestOps::envelope_type, envelope_type> && ...)
       , "one envelope type for all operators");

      using config = fm_voice_config;
      using note = basic_fm_note<envelope_type>;

                     basic_fm_voice();
                     basic_fm_voice(config const& cfg, float sps);

                     // Defined here: the constraint names `note`, which
                     // an out-of-class definition cannot spell the same.
                     template <concepts::Patcher<note> P>
                     basic_fm_voice(P const& patch)
                      : basic_fm_voice{patch.voice, patch.sps}
                     {}

      void           set(config const& cfg, float sps);
      void           update(config const& cfg, note const& n);
                     template <concepts::Patcher<note> P>
      void           update(
                        P const& patch, std::uint8_t key, float velocity)
                     {
                        note n;
                        patch.note(key, velocity, n);
                        update(patch.voice, n);
                     }

      void           attack(note const& n);
                     template <concepts::Patcher<note> P>
      void           attack(
                        P const& patch, std::uint8_t key, float velocity)
                     {
                        note n;
                        patch.note(key, velocity, n);
                        attack(n);
                     }
      void           release();
      bool           active() const;

      void           mod_wheel(float w)      { _wheel = w; }
      float          mod_wheel() const       { return _wheel; }
      void           enable(fm_routing::mask m) { _enabled = m; }
      fm_routing::mask
                     enabled() const         { return _enabled; }

      float          operator()(phase_iterator master);

   private:

      using operators = std::tuple<Op, RestOps...>;
      using pitch_env = detail::fm_pitch_env;

      void           configure(config const& cfg);
      void           apply(note const& n);

      operators      _op;
      fm_algorithm   _alg;
      lfo_gen        _lfo;
      pitch_env      _pitch_env;
      float          _sps;
      float          _pitch_mod;
      float          _amp_mod[size];
      float          _pitch_mod_wheel;
      float          _amp_mod_wheel[size];
      bool           _key_sync;
      float          _semis = 0.0f;   // the pitch offset last applied
      float          _bend = 1.0f;
      float          _wheel = 0.0f;
      fm_routing::mask
                     _enabled = fm_routing::mask(~0u);
   };

   using fm_voice = basic_fm_voice<
      fm_operator, fm_operator, fm_operator
    , fm_operator, fm_operator, fm_operator>;

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      // f(op, index) over a tuple of operators
      template <typename Tuple, typename F>
      inline void fm_for_each(Tuple& ops, F f)
      {
         std::apply([&](auto&... op)
         {
            std::size_t i = 0;
            (f(op, i++), ...);
         }, ops);
      }

      inline void fm_pitch_env::set(
         std::array<float, 4> const& semis
       , std::array<float, 4> const& rate
       , float sps
      )
      {
         retarget(semis, rate, sps);
         _semis = semis[3];              // rests at L4
         _i = 4;
      }

      inline void fm_pitch_env::retarget(
         std::array<float, 4> const& semis
       , std::array<float, 4> const& rate
       , float sps
      )
      {
         _target = semis;
         for (std::size_t i = 0; i != 4; ++i)
            _speed[i] = rate[i] / sps;
      }

      inline float fm_pitch_env::operator()()
      {
         if (_i == 4)
            return _semis;

         auto target = _target[_i];
         if (_semis < target)
            _semis = std::min(_semis + _speed[_i], target);
         else
            _semis = std::max(_semis - _speed[_i], target);
         if (_semis == target)
            _i = (_i < 2) ? _i + 1 : 4;
         return _semis;
      }
   }

   template <typename Op, typename... RestOps>
   inline basic_fm_voice<Op, RestOps...>::basic_fm_voice()
    : basic_fm_voice{config{}, 44100.0f}
   {}

   template <typename Op, typename... RestOps>
   inline basic_fm_voice<Op, RestOps...>::basic_fm_voice(
      config const& cfg, float sps)
   {
      set(cfg, sps);
   }

   // What set and update share: none of it restarts anything. The
   // algorithm and the LFO take their settings; their state carries on.
   template <typename Op, typename... RestOps>
   inline void basic_fm_voice<Op, RestOps...>::configure(config const& cfg)
   {
      _alg.set(cfg.algorithm);
      _lfo.set(cfg.lfo, _sps);
      _pitch_mod = cfg.pitch_mod;
      std::copy(cfg.amp_mod, cfg.amp_mod + size, _amp_mod);
      _pitch_mod_wheel = cfg.pitch_mod_wheel;
      std::copy(cfg.amp_mod_wheel, cfg.amp_mod_wheel + size, _amp_mod_wheel);
      _key_sync = cfg.key_sync;
   }

   // Each operator's pitch and envelope settings from a note; an
   // envelope takes new rates and levels where it is.
   template <typename Op, typename... RestOps>
   inline void basic_fm_voice<Op, RestOps...>::apply(note const& n)
   {
      detail::fm_for_each(_op, [&](auto& op, std::size_t i)
      {
         auto const& p = n.op[i];
         if (p.ratio != 0.0)
            op.ratio(p.ratio);
         else
            op.fixed(p.fixed_step);
         op.envelope(p.env, _sps);
      });
   }

   template <typename Op, typename... RestOps>
   inline void basic_fm_voice<Op, RestOps...>::set(
      config const& cfg, float sps)
   {
      _sps = sps;
      configure(cfg);
      _pitch_env.set(cfg.pitch_env_level, cfg.pitch_env_rate, sps);
   }

   template <typename Op, typename... RestOps>
   inline void basic_fm_voice<Op, RestOps...>::update(
      config const& cfg, note const& n)
   {
      configure(cfg);
      _pitch_env.retarget(cfg.pitch_env_level, cfg.pitch_env_rate, _sps);
      apply(n);
   }

   template <typename Op, typename... RestOps>
   inline void basic_fm_voice<Op, RestOps...>::attack(note const& n)
   {
      apply(n);
      detail::fm_for_each(_op, [&](auto& op, std::size_t)
      {
         if (_key_sync)
            op.sync();
         op.attack();
      });
      if (_key_sync)
         _alg.sync();
      _pitch_env.attack();
      _lfo.key_on();
   }

   template <typename Op, typename... RestOps>
   inline void basic_fm_voice<Op, RestOps...>::release()
   {
      detail::fm_for_each(_op, [](auto& op, std::size_t) { op.release(); });
      _pitch_env.release();
   }

   template <typename Op, typename... RestOps>
   inline bool basic_fm_voice<Op, RestOps...>::active() const
   {
      bool any = false;
      detail::fm_for_each(_op, [&](auto const& op, std::size_t)
      {
         any = any || op.active();
      });
      return any;
   }

   template <typename Op, typename... RestOps>
   inline float basic_fm_voice<Op, RestOps...>::operator()(
      phase_iterator master)
   {
      _lfo();

      // Pitch: the LFO by its depth, or the wheel's if deeper, plus the
      // pitch envelope, as a factor on the master's step, recomputed only
      // when it moves.
      auto depth = std::max(_pitch_mod, _wheel * _pitch_mod_wheel);
      auto semis = _lfo.value() * depth + _pitch_env();
      if (semis != _semis)
      {
         _semis = semis;
         _bend = std::exp2(semis / 12.0f);
      }
      if (_bend != 1.0f)
         master._step = phase{
            std::uint32_t(master._step.rep * _bend), direct_unit};

      // Amplitude: each operator's cut at the LFO's trough, in dB, by its
      // depth or the wheel's; nothing at all from one switched off
      auto trough = (1.0f - _lfo.value()) * 0.5f;   // 0 at the LFO's peak
      detail::fm_for_each(_op, [&](auto& op, std::size_t i)
      {
         if (!(_enabled & (1u << i)))
         {
            op.gain(0.0f);
            return;
         }
         auto cut = std::max(_amp_mod[i], _wheel * _amp_mod_wheel[i]);
         op.gain(cut > 0.0f ? lin_float(dB(-cut * trough)) : 1.0f);
      });

      return _alg(_op, master);
   }
}

#endif
