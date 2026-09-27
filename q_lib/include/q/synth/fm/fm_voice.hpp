/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
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
                     template <concepts::Patcher<note> P>
                     basic_fm_voice(P const& patch);

      void           set(config const& cfg, float sps);

      void           attack(note const& n);
                     template <concepts::Patcher<note> P>
      void           attack(
                        P const& patch, std::uint8_t key, float velocity
                     );
      void           release();
      bool           active() const;

      float          operator()(phase_iterator master);

   private:

      using operators = std::tuple<Op, RestOps...>;
      using pitch_env = detail::fm_pitch_env;

      operators      _op;
      fm_algorithm   _alg;
      lfo_gen        _lfo;
      pitch_env      _pitch_env;
      float          _sps;
      float          _pitch_mod;
      float          _amp_mod[size];
      bool           _key_sync;
      float          _semis = 0.0f;   // the pitch offset last applied
      float          _bend = 1.0f;
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
         _target = semis;
         for (std::size_t i = 0; i != 4; ++i)
            _speed[i] = rate[i] / sps;
         _semis = semis[3];              // rests at L4
         _i = 4;
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

   template <typename Op, typename... RestOps>
   template <concepts::Patcher<basic_fm_note<
      typename basic_fm_voice<Op, RestOps...>::envelope_type>> P>
   inline basic_fm_voice<Op, RestOps...>::basic_fm_voice(P const& patch)
    : basic_fm_voice{patch.voice, patch.sps}
   {}

   template <typename Op, typename... RestOps>
   inline void basic_fm_voice<Op, RestOps...>::set(
      config const& cfg, float sps)
   {
      _sps = sps;
      _alg.set(cfg.algorithm);
      _lfo.set(cfg.lfo, sps);
      _pitch_env.set(cfg.pitch_env_level, cfg.pitch_env_rate, sps);
      _pitch_mod = cfg.pitch_mod;
      std::copy(cfg.amp_mod, cfg.amp_mod + size, _amp_mod);
      _key_sync = cfg.key_sync;
   }

   template <typename Op, typename... RestOps>
   inline void basic_fm_voice<Op, RestOps...>::attack(note const& n)
   {
      detail::fm_for_each(_op, [&](auto& op, std::size_t i)
      {
         auto const& p = n.op[i];
         if (p.ratio != 0.0)
            op.ratio(p.ratio);
         else
            op.fixed(p.fixed_step);
         op.envelope(p.env, _sps);
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
   template <concepts::Patcher<basic_fm_note<
      typename basic_fm_voice<Op, RestOps...>::envelope_type>> P>
   inline void basic_fm_voice<Op, RestOps...>::attack(
      P const& patch, std::uint8_t key, float velocity)
   {
      note n;
      patch.note(key, velocity, n);
      attack(n);
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

      // Pitch: the LFO by its depth plus the pitch envelope, as a factor
      // on the master's step, recomputed only when it moves.
      auto semis = _lfo.value() * _pitch_mod + _pitch_env();
      if (semis != _semis)
      {
         _semis = semis;
         _bend = std::exp2(semis / 12.0f);
      }
      if (_bend != 1.0f)
         master._step = phase{
            std::uint32_t(master._step.rep * _bend), direct_unit};

      // Amplitude: each operator's cut at the LFO's trough, in dB
      auto trough = (1.0f - _lfo.value()) * 0.5f;   // 0 at the LFO's peak
      detail::fm_for_each(_op, [&](auto& op, std::size_t i)
      {
         op.gain(_amp_mod[i] > 0.0f ?
            lin_float(dB(-_amp_mod[i] * trough)) : 1.0f);
      });

      return _alg(_op, master);
   }
}

#endif
