/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SIGNAL_CONDITIONER_HPP_MARCH_12_2018)
#define CYCFI_Q_SIGNAL_CONDITIONER_HPP_MARCH_12_2018

#include <q/support/literals.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/clip.hpp>
#include <q/fx/noise_gate.hpp>
#include <q/fx/lowpass.hpp>
#include <q/fx/biquad.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // signal_conditioner preprocesses and enhances a signal for analytical
   // processes such as onset and pitch detection.
   ////////////////////////////////////////////////////////////////////////////
   class signal_conditioner
   {
   public:

      struct config
      {
         // Pre clip
         decibel              pre_clip_level          = -10_dB;

         // Compressor
         decibel              comp_threshold          = -27_dB;
         float                comp_slope              = 1.0/6;
         float                comp_gain               = 10;

         // Gate
         duration             attack_width            = 500_us;
         decibel              gate_onset_threshold    = -33_dB;
         decibel              gate_release_threshold  = -55_dB;
         duration             gate_release            = 10_ms;
      };

                              template <typename Config>
                              signal_conditioner(
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

      using noise_gate = basic_noise_gate<50>;

      clip                    _clip;
      highpass                _hp;
      dynamic_smoother        _sm;
      fast_envelope_follower  _env;
      float                   _post_env;
      compressor              _comp;
      float                   _makeup_gain;
      noise_gate              _gate;
      envelope_follower       _gate_env;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Config>
   inline signal_conditioner::signal_conditioner(
      Config const& conf
    , frequency lowest_freq
    , frequency highest_freq
    , float sps
   )
    : _clip{conf.pre_clip_level}
    , _hp{lowest_freq, sps}
    , _sm{lowest_freq + ((highest_freq - lowest_freq) / 2), sps}
    , _env{lowest_freq.period()*0.6, sps}
    , _comp{conf.comp_threshold, conf.comp_slope}
    , _makeup_gain{conf.comp_gain}
    , _gate{
         conf.gate_onset_threshold
       , conf.gate_release_threshold
       , conf.attack_width
       , sps
      }
    , _gate_env{500_us, conf.gate_release, sps}
   {}

   inline float signal_conditioner::operator()(float s)
   {
      // High pass
      s = _hp(s);

      // Pre clip
      s = _clip(s);

      // Dynamic Smoother
      s = _sm(s);

      // Signal envelope
      auto env = _env(std::abs(s));

      // Noise gate
      auto gate = _gate(env);
      s *= _gate_env(gate);

      // Compressor + makeup-gain
      auto env_db = decibel(env);
      auto gain = as_float(_comp(env_db)) * _makeup_gain;
      s = s * gain;
      _post_env = env * gain;

      return s;
   }

   inline bool signal_conditioner::gate() const
   {
      return _gate();
   }

   inline float signal_conditioner::gate_env() const
   {
      return _gate_env();
   }

   inline float signal_conditioner::pre_env() const
   {
      return _env();
   }

   inline float signal_conditioner::signal_env() const
   {
      return _post_env;
   }

   inline void signal_conditioner::onset_threshold(decibel onset_threshold)
   {
      _gate.onset_threshold(onset_threshold);
   }

   inline void signal_conditioner::release_threshold(decibel release_threshold)
   {
      _gate.release_threshold(release_threshold);
   }

   inline void signal_conditioner::onset_threshold(float onset_threshold)
   {
      _gate.onset_threshold(onset_threshold);
   }

   inline void signal_conditioner::release_threshold(float release_threshold)
   {
      _gate.release_threshold(release_threshold);
   }
}

#endif

