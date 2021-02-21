/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SIGNAL_CONDITIONER_HPP_MARCH_12_2018)
#define CYCFI_Q_SIGNAL_CONDITIONER_HPP_MARCH_12_2018

#include <q/support/literals.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/waveshaper.hpp>
#include <q/fx/moving_average.hpp>
#include <q/fx/noise_gate.hpp>
#include <q/fx/lowpass.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // signal_conditioner preprocesses and enhances a signal for analytical
   // processes such as pitch detection.
   ////////////////////////////////////////////////////////////////////////////
   class signal_conditioner
   {
   public:

      struct config
      {
         // AGC
         decibel              agc_level = -3_dB;
         decibel              agc_max_gain = 32_dB;

         // Gate
         decibel              gate_release_threshold = -45_dB;
         duration             gate_release = 10_ms;
      };

                              template <typename Config>
                              signal_conditioner(
                                 Config const& conf
                               , std::uint32_t sps
                              );

      float                   operator()(float s);
      bool                    gate() const;
      float                   gate_env() const;

   private:

      agc                     _agc;
      decibel                 _agc_level;
      noise_gate              _gate;
      envelope_follower       _gate_env;
      one_pole_lowpass        _gain_lp;
      clip                    _clip;
   };

   ////////////////////////////////////////////////////////////////////////////
   // bl_signal_conditioner signal_conditioner with bandpass filter to limit
   // the range within a lower and upper frequency bounds.
   ////////////////////////////////////////////////////////////////////////////
   class bl_signal_conditioner : public signal_conditioner
   {
   public:

      using config = signal_conditioner::config;

      template <typename Config>
      bl_signal_conditioner(
         Config const& conf
       , frequency lowest_freq
       , frequency highest_freq
       , std::uint32_t sps
      )
       : signal_conditioner{conf, sps}
       , _lp1{highest_freq * 2, sps}
       , _lp2{lowest_freq / 2, sps}
      {}

      float operator()(float s)
      {
         // Bandpass filter
         s = _lp1(s);
         s -= _lp2(s);

         return signal_conditioner::operator()(s);
      }

   private:

      one_pole_lowpass        _lp1;
      one_pole_lowpass        _lp2;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Config>
   inline signal_conditioner::signal_conditioner(
      Config const& conf
    , std::uint32_t sps
   )
    : _agc{conf.agc_max_gain}
    , _agc_level{conf.agc_level}
    , _gate{conf.gate_release_threshold, sps}
    , _gate_env{500_us, conf.gate_release, sps}
    , _gain_lp{20_Hz, sps}
    , _clip{conf.agc_level}
   {
   }

   inline float signal_conditioner::operator()(float s)
   {
      // Noise gate
      auto gate = _gate(s);
      s *= _gate_env(gate);

      // AGC
      auto env_db = decibel(_gate.env());
      auto gain = float(_agc(env_db, _agc_level));
      s *= _gain_lp(gain);

      // Hard clip
      return _clip(s);
   }

   inline bool signal_conditioner::gate() const
   {
      return _gate();
   }

   inline float signal_conditioner::gate_env() const
   {
      return _gate_env();
   }
}

#endif

