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
   // Stages of the conditioner that can be bypassed at compile time, named
   // in chain order. Combine with |:
   // basic_signal_conditioner<sc_bypass::clip | sc_bypass::compressor>.
   ////////////////////////////////////////////////////////////////////////////
   struct sc_bypass
   {
      static constexpr unsigned none       = 0;
      static constexpr unsigned highpass   = 1 << 0;
      static constexpr unsigned smoother   = 1 << 1;
      static constexpr unsigned clip       = 1 << 2;
      static constexpr unsigned compressor = 1 << 3;
   };

   ////////////////////////////////////////////////////////////////////////////
   // basic_signal_conditioner preprocesses a signal for analytical
   // processes such as onset and pitch detection. The chain, in order:
   //
   //    highpass -> smoother -> clip -> envelope -> noise gate
   //             -> compressor + makeup
   //
   // Bypass is a mask of stages (sc_bypass): a bypassed stage costs neither
   // work nor storage (see q::bypassable). The clip and compressor are
   // bypassed by consumers that measure crest timing or shape; the highpass
   // and smoother by a host whose own front-end (a decimating FIR) already
   // does their job. The envelope and gate are unconditional: everything
   // reads gate().
   //
   // The order is load-bearing and never changes. Taps are defined by
   // POSITION, not by which stages ran: pre_env() is the envelope before the
   // compressor, signal_env() the envelope of the output.
   //
   // signal_conditioner is the full chain.
   ////////////////////////////////////////////////////////////////////////////
   template <unsigned Bypass = sc_bypass::none>
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

      void                    onset_threshold(decibel onset_threshold);
      void                    release_threshold(decibel release_threshold);
      void                    onset_threshold(float onset_threshold);
      void                    release_threshold(float release_threshold);

   private:

      static constexpr bool bypass_highpass =
         (Bypass & sc_bypass::highpass) != 0;
      static constexpr bool bypass_smoother =
         (Bypass & sc_bypass::smoother) != 0;
      static constexpr bool bypass_clip =
         (Bypass & sc_bypass::clip) != 0;
      static constexpr bool bypass_compressor =
         (Bypass & sc_bypass::compressor) != 0;

      using hp_stage   = bypassable<bypass_highpass, highpass>;
      using sm_stage   = bypassable<bypass_smoother, dynamic_smoother>;
      using clip_stage = bypassable<bypass_clip, tanh_clip>;
      using comp_stage = bypassable<bypass_compressor, compressor>;

      clip_stage              _clip;
      hp_stage                _hp;
      sm_stage                _sm;
      fast_envelope_follower  _env;
      peak_envelope_follower  _env_lp;
      float                   _post_env;
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
   template <unsigned Bypass>
   template <typename Config>
   inline basic_signal_conditioner<Bypass>
      ::basic_signal_conditioner(
      Config const& conf
    , frequency lowest_freq
    , frequency highest_freq
    , float sps
   )
    : _clip{make_bypassable<bypass_clip, tanh_clip>(conf.pre_clip_level)}
    , _hp{make_bypassable<bypass_highpass, highpass>(lowest_freq, sps)}
    , _sm{make_bypassable<bypass_smoother, dynamic_smoother>(
         lowest_freq + ((highest_freq - lowest_freq) / 2), sps)}
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

   template <unsigned Bypass>
   inline float
   basic_signal_conditioner<Bypass>
      ::operator()(float s)
   {
      // High pass
      if constexpr (!bypass_highpass)
         s = _hp(s);

      // Dynamic Smoother
      if constexpr (!bypass_smoother)
         s = _sm(s);

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

   template <unsigned Bypass>
   inline bool basic_signal_conditioner<Bypass>
      ::gate() const
   {
      return _gate();
   }

   template <unsigned Bypass>
   inline float basic_signal_conditioner<Bypass>
      ::gate_env() const
   {
      return _gate_env();
   }

   template <unsigned Bypass>
   inline float basic_signal_conditioner<Bypass>
      ::pre_env() const
   {
      return _env_lp();
   }

   template <unsigned Bypass>
   inline float basic_signal_conditioner<Bypass>
      ::signal_env() const
   {
      return _post_env;
   }

   template <unsigned Bypass>
   inline void basic_signal_conditioner<Bypass>
      ::onset_threshold(decibel onset_threshold)
   {
      _gate.onset_threshold(onset_threshold);
   }

   template <unsigned Bypass>
   inline void basic_signal_conditioner<Bypass>
      ::release_threshold(decibel release_threshold)
   {
      _gate.release_threshold(release_threshold);
   }

   template <unsigned Bypass>
   inline void basic_signal_conditioner<Bypass>
      ::onset_threshold(float onset_threshold)
   {
      _gate.onset_threshold(onset_threshold);
   }

   template <unsigned Bypass>
   inline void basic_signal_conditioner<Bypass>
      ::release_threshold(float release_threshold)
   {
      _gate.release_threshold(release_threshold);
   }
}

#endif
