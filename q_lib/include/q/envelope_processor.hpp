/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_ENVELOPE_PROCESSOR_HPP_NOVEMBER_9_2018)
#define CYCFI_Q_ENVELOPE_PROCESSOR_HPP_NOVEMBER_9_2018

#include <q/literals.hpp>
#include <q/sfx.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // envelope_processor
   ////////////////////////////////////////////////////////////////////////////
   struct envelope_processor
   {
      static constexpr float hysteresis = 0.0001; // -80dB

      struct config
      {
         // Onset detector
         double               onset_sensitivity    = 0.8;
         duration             onset_decay          = 100_ms;
         decibel              release_threshold    = -30_dB;

         // Compressor
         duration             comp_release         = 30_ms;
         decibel              comp_threshold       = -18_dB;
         double               comp_slope           = 1.0/4;
         double               comp_gain            = 3;

         // Gate
         decibel              gate_on_threshold    = -36_dB;
         decibel              gate_off_threshold   = -60_dB;

         duration             decay                = 300_ms;
      };

                              envelope_processor(std::uint32_t sps);
                              envelope_processor(config const& conf, std::uint32_t sps);
      float                   operator()(float s);

      onset_detector          _pos_onset;
      onset_detector          _neg_onset;
      peak_envelope_follower  _env;
      compressor              _comp;
      window_comparator       _gate;
      float                   _release_threshold;
      float                   _end_release;
      float                   _makeup_gain;

      float                   _decay;
      float                   _y = 0.0f;

      one_pole_lowpass        _lp;

   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline envelope_processor::envelope_processor(config const& conf, std::uint32_t sps)
    : _pos_onset(conf.onset_sensitivity, conf.onset_decay, sps)
    , _neg_onset(conf.onset_sensitivity, conf.onset_decay, sps)
    , _env(conf.comp_release, sps)
    , _comp(conf.comp_threshold, conf.comp_slope)
    , _gate(float(conf.gate_off_threshold), float(conf.gate_on_threshold))
    , _release_threshold(conf.release_threshold)
    , _makeup_gain(conf.comp_gain)
    , _lp(conf.onset_decay, sps)
    , _decay(fast_exp3(-2.0f / (sps * double(conf.decay))))
   {}

   inline envelope_processor::envelope_processor(std::uint32_t sps)
    : envelope_processor(config{}, sps)
   {}

   inline float envelope_processor::operator()(float s)
   {
      // Main envelope
      auto env = _env(std::abs(s));

      // Noise gate
      if (_gate(env))
      {
         // Compressor + makeup-gain + hard clip
         constexpr clip _clip;
         s = _clip(_comp(s, env) * _makeup_gain);
      }
      else
      {
         s = 0.0f;
      }

      auto pos_onset = _pos_onset(s > 0.0? s : 0.0f);
      auto neg_onset = _neg_onset(s < 0.0f? -s : 0.0f);
      auto peak = std::max(pos_onset, neg_onset);

      if (peak > _y)
      {
         _y = peak;
      }
      else
      {
         auto level = _pos_onset._lp(); // std::max(_pos_onset._lp(), _neg_onset._lp());
         _y = level + _decay * (_y - level);
         if (_y < level + hysteresis)
            _y = level;
      }

      return _y;

      // return _onset._env();

      // float r;

      // if (onset != 0.0f)
      // {
      //    r = onset; // _onset._env();
      // }
      // else
      // {
      //    r = _onset._lp();
      // }

      // return r;
   }
}}

#endif
