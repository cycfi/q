/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PD_PREPROCESSOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PD_PREPROCESSOR_HPP_MARCH_12_2018

#include <q/support/literals.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/waveshaper.hpp>
#include <q/fx/moving_average.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   class pd_preprocessor
   {
   public:

      struct config
      {
         // Compressor
         duration             comp_release            = 30_ms;
         decibel              comp_threshold          = -24_dB;
         double               comp_slope              = 1.0/4;
         double               comp_gain               = 8;

         // Gate
         decibel              gate_on_threshold       = -30_dB;
         decibel              gate_off_threshold      = -45_dB;
      };

                              template <typename Config>
                              pd_preprocessor(
                                 Config const& conf
                               , frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                              );

      float                   operator()(float s);
      bool                    gate() const               { return _gate(); }

   private:

      peak_envelope_follower  _env;
      compressor              _comp;
      window_comparator       _gate;
      one_pole_lowpass        _lp1;
      one_pole_lowpass        _lp2;
      moving_average          _ma{ 4 };

      float                   _makeup_gain;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Config>
   inline pd_preprocessor::pd_preprocessor(
      Config const& conf
    , q::frequency lowest_freq
    , q::frequency highest_freq
    , std::uint32_t sps
   )
    : _env(conf.comp_release, sps)
    , _comp(conf.comp_threshold, conf.comp_slope)
    , _gate(float(conf.gate_off_threshold), float(conf.gate_on_threshold))
    , _lp1(highest_freq * 2, sps)
    , _lp2(lowest_freq / 2, sps)
    , _makeup_gain(conf.comp_gain)
   {
   }

   inline float pd_preprocessor::operator()(float s)
   {
      // Bandpass filter
      s = _lp1(s);
      s -= _lp2(s);

      // Envelope
      auto env = q::decibel(_env(std::abs(s)));

      // Noise gate
      if (_gate(float(env)))
      {
         // Compressor + makeup-gain + hard clip
         constexpr clip _clip;
         auto gain = float(_comp(env)) * _makeup_gain;
         s = _clip(s * gain);
      }
      else
      {
         s = 0.0f;
      }

      return _ma(s);
   }
}

#endif

