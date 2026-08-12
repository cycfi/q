/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_SIGNAL_CONDITIONER_HPP_MARCH_12_2018)
#define CYCFI_Q_SIGNAL_CONDITIONER_HPP_MARCH_12_2018

#include <q/support/literals.hpp>
#include <q/support/bypassable.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/clip.hpp>
#include <q/fx/onset_gate.hpp>
#include <q/fx/lowpass.hpp>
#include <q/fx/biquad.hpp>
#include <q/fx/envelope.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // signal_conditioner_config does not depend on which stages are compiled
   // in, so it lives outside the template: one config type, not one per
   // specialization. basic_signal_conditioner::config names it, so
   // signal_conditioner::config keeps working.
   ////////////////////////////////////////////////////////////////////////////
   struct signal_conditioner_config
   {
      // Pre clip
      decibel                 pre_clip_level          = -10_dB;

      // Compressor
      decibel                 comp_threshold          = -27_dB;
      float                   comp_slope              = 1.0/6;
      float                   comp_gain               = 10;

      // Gate. gate_onset_threshold is the fast-open level; slope_threshold
      // is the rise over attack_width.
      duration                attack_width            = 500_us;
      decibel                 gate_onset_threshold    = -24_dB;
      decibel                 slope_threshold         = -45_dB;
      decibel                 gate_release_threshold  = -55_dB;
      duration                gate_release            = 10_ms;
   };

   ////////////////////////////////////////////////////////////////////////////
   // basic_signal_conditioner preprocesses and enhances a signal for
   // analytical processes such as onset and pitch detection.
   //
   // The chain is, in order:
   //
   //    highpass -> dynamic smoother -> [smoothed() tap] -> clip
   //             -> envelope -> noise gate -> compressor + makeup
   //
   // The clip and the compressor can each be bypassed at compile time. They
   // are the two stages with a real use for being absent: analytical
   // consumers that measure crest timing or shape want neither, because the
   // clip saturates crests and the compressor's time-varying gain shifts
   // their apexes. The rest of the chain has no such case -- everything reads
   // gate(), and the highpass and smoother are what make the signal usable at
   // all -- so they are unconditional, which keeps this class simple and the
   // test matrix at four configurations.
   //
   // What bypassing buys is the WORK: dropping the compressor takes a
   // lin_to_db, a compressor evaluation and two multiplies out of every
   // sample. Storage is a minor bonus and not a guarantee -- bypassing the
   // compressor saves 16 bytes of 208, while bypassing the clip saves
   // nothing at all, its 8 bytes going to alignment padding instead (see
   // q::bypassable).
   //
   // The stages are switchable but never reorderable. The order above is
   // load-bearing -- the smoother must precede the clip, or the clip
   // saturates crests the smoother would have resolved.
   //
   // Taps are defined by POSITION in the chain, not by which stages ran:
   //
   //    smoothed()    the signal before the clip. Bypassing the clip or the
   //                  compressor cannot change it.
   //    pre_env()     the envelope before the compressor's makeup.
   //    signal_env()  the envelope of the OUTPUT: pre_env() times the makeup
   //                  gain, or simply pre_env() when the compressor is
   //                  bypassed.
   //
   // signal_conditioner is the default specialization -- the full chain, the
   // behavior this class has always had.
   ////////////////////////////////////////////////////////////////////////////
   template <bool bypass_clip = false, bool bypass_compressor = false>
   class basic_signal_conditioner
   {
   public:

      using config = signal_conditioner_config;

                              template <typename Config>
                              basic_signal_conditioner(
                                 Config const& conf
                               , frequency lowest_freq
                               , frequency highest_freq
                               , float sps
                              );

      float                   operator()(float s);
      bool                    gate() const;
      float                   gate_env() const;
      float                   pre_env() const;
      float                   signal_env() const;
      float                   smoothed() const;

      void                    onset_threshold(decibel onset_threshold);
      void                    release_threshold(decibel release_threshold);
      void                    onset_threshold(float onset_threshold);
      void                    release_threshold(float release_threshold);

   private:

      using clip_stage = bypassable<bypass_clip, tanh_clip>;
      using comp_stage = bypassable<bypass_compressor, compressor>;

      clip_stage              _clip;
      highpass                _hp;
      dynamic_smoother        _sm;
      fast_envelope_follower  _env;
      peak_envelope_follower  _env_lp;
      float                   _post_env;
      float                   _smoothed = 0.0f;
      comp_stage              _comp;
      float                   _makeup_gain;
      onset_gate              _gate;
      ar_envelope_follower    _gate_env;
   };

   ////////////////////////////////////////////////////////////////////////////
   // signal_conditioner: the full chain. The name and behavior dependent code
   // has always relied on.
   ////////////////////////////////////////////////////////////////////////////
   using signal_conditioner = basic_signal_conditioner<>;

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <bool bypass_clip, bool bypass_compressor>
   template <typename Config>
   inline basic_signal_conditioner<bypass_clip, bypass_compressor>
      ::basic_signal_conditioner(
      Config const& conf
    , frequency lowest_freq
    , frequency highest_freq
    , float sps
   )
    : _clip{make_bypassable<bypass_clip, tanh_clip>(conf.pre_clip_level)}
    , _hp{lowest_freq, sps}
    , _sm{lowest_freq + ((highest_freq - lowest_freq) / 2), sps}
    , _env{lowest_freq.period()*0.6, sps}
    , _env_lp{lowest_freq.period(), sps}
    , _comp{make_bypassable<bypass_compressor, compressor>(
         conf.comp_threshold, conf.comp_slope)}
    , _makeup_gain{conf.comp_gain}
    , _gate{
         conf.gate_onset_threshold
       , conf.slope_threshold
       , conf.gate_release_threshold
       , conf.attack_width
       , sps
      }
    , _gate_env{conf.attack_width, conf.gate_release, sps}
   {}

   template <bool bypass_clip, bool bypass_compressor>
   inline float
   basic_signal_conditioner<bypass_clip, bypass_compressor>::operator()(float s)
   {
      // High pass
      s = _hp(s);

      // Dynamic Smoother
      s = _sm(s);

      // Smoothed tap: cleaned but not yet clipped or compressed. The
      // clip saturates crests and the compressor's time-varying gain
      // shifts their apexes, so timing analyses (e.g. peak picking)
      // read this tap instead of the conditioned output.
      _smoothed = s;

      // Pre clip
      if constexpr (!bypass_clip)
         s = _clip(s);

      // Signal envelope
      auto env = _env_lp(_env(std::abs(s)));

      // Noise gate
      auto gate = _gate(env);
      s *= _gate_env(gate);

      // Compressor + makeup-gain
      if constexpr (!bypass_compressor)
      {
         auto env_db = lin_to_db(env);
         auto gain = lin_float(_comp(env_db)) * _makeup_gain;
         s = s * gain;
         _post_env = env * gain;
      }
      else
      {
         _post_env = env;
      }

      return s;
   }

   template <bool bypass_clip, bool bypass_compressor>
   inline bool basic_signal_conditioner<bypass_clip, bypass_compressor>
      ::gate() const
   {
      return _gate();
   }

   template <bool bypass_clip, bool bypass_compressor>
   inline float basic_signal_conditioner<bypass_clip, bypass_compressor>
      ::gate_env() const
   {
      return _gate_env();
   }

   template <bool bypass_clip, bool bypass_compressor>
   inline float basic_signal_conditioner<bypass_clip, bypass_compressor>
      ::pre_env() const
   {
      return _env_lp();
   }

   template <bool bypass_clip, bool bypass_compressor>
   inline float basic_signal_conditioner<bypass_clip, bypass_compressor>
      ::smoothed() const
   {
      return _smoothed;
   }

   template <bool bypass_clip, bool bypass_compressor>
   inline float basic_signal_conditioner<bypass_clip, bypass_compressor>
      ::signal_env() const
   {
      return _post_env;
   }

   template <bool bypass_clip, bool bypass_compressor>
   inline void basic_signal_conditioner<bypass_clip, bypass_compressor>
      ::onset_threshold(decibel onset_threshold)
   {
      _gate.onset_threshold(onset_threshold);
   }

   template <bool bypass_clip, bool bypass_compressor>
   inline void basic_signal_conditioner<bypass_clip, bypass_compressor>
      ::release_threshold(decibel release_threshold)
   {
      _gate.release_threshold(release_threshold);
   }

   template <bool bypass_clip, bool bypass_compressor>
   inline void basic_signal_conditioner<bypass_clip, bypass_compressor>
      ::onset_threshold(float onset_threshold)
   {
      _gate.onset_threshold(onset_threshold);
   }

   template <bool bypass_clip, bool bypass_compressor>
   inline void basic_signal_conditioner<bypass_clip, bypass_compressor>
      ::release_threshold(float release_threshold)
   {
      _gate.release_threshold(release_threshold);
   }
}

#endif
