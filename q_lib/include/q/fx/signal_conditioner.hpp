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
         // Compressor
         duration             comp_release            = 30_ms;
         decibel              comp_threshold          = -24_dB;
         float                comp_slope              = 1.0/4;
         float                comp_gain               = 8;

         // Gate
         decibel              gate_release_threshold  = -45_dB;
         duration             gate_release            = 10_ms;
      };

                              template <typename Config>
                              signal_conditioner(
                                 Config const& conf
                               , std::uint32_t sps
                              );

      float                   operator()(float s);
      noise_gate const&       gate() const;

   private:

      compressor              _comp;
      noise_gate              _gate;
      envelope_follower       _gate_env;
      moving_average          _ma{4};
      float                   _makeup_gain;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Config>
   inline signal_conditioner::signal_conditioner(
      Config const& conf
    , std::uint32_t sps
   )
    : _comp{conf.comp_threshold, conf.comp_slope}
    , _gate{conf.gate_release_threshold, sps}
    , _gate_env{500_us, conf.gate_release, sps}
    , _makeup_gain{conf.comp_gain}
   {
   }

   inline float signal_conditioner::operator()(float s)
   {
      // Noise gate
      auto gate = _gate(s);
      s *= _gate_env(gate);

      // Compressor + makeup-gain + hard clip
      constexpr clip _clip;
      auto env_db = decibel(_gate.env());
      auto gain = float(_comp(env_db)) * _makeup_gain;
      s = _clip(s * gain);

      return _ma(s);
   }

   inline noise_gate const& signal_conditioner::gate() const
   {
      return _gate;
   }
}

#endif

