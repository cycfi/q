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
   class envelope_processor
   {
   public:

      static constexpr float hysteresis = 0.0001; // -80dB

      struct config
      {
         // Onset detector
         double               onset_sensitivity    = 0.8;
         duration             onset_decay          = 100_ms;

         // Compressor
         duration             comp_release         = 30_ms;
         decibel              comp_threshold       = -18_dB;
         double               comp_slope           = 1.0/4;
         double               comp_gain            = 3;

         // Gate
         decibel              gate_on_threshold    = -36_dB;
         decibel              gate_off_threshold   = -60_dB;

         // Attack / Decay
         duration             attack               = 100_ms;
         duration             decay                = 300_ms;
      };

                              envelope_processor(std::uint32_t sps);
                              envelope_processor(config const& conf, std::uint32_t sps);

      float                   operator()(float s);
      float                   envelope() const     { return _y; }
      bool                    is_note_on() const   { return _is_note_on; }

   private:

      onset_detector          _onset;
      peak_envelope_follower  _env;
      compressor              _comp;
      window_comparator       _gate;

      float                   _end_release;
      float                   _makeup_gain;

      bool                    _is_note_on = false;
      float                   _attack;
      float                   _decay;
      float                   _peak = 0.0f;
      float                   _y = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline envelope_processor::envelope_processor(config const& conf, std::uint32_t sps)
    : _onset(conf.onset_sensitivity, conf.onset_decay, sps)
    , _env(conf.comp_release, sps)
    , _comp(conf.comp_threshold, conf.comp_slope)
    , _gate(float(conf.gate_off_threshold), float(conf.gate_on_threshold))
    , _makeup_gain(conf.comp_gain)
    , _attack(fast_exp3(-2.0f / (sps * double(conf.attack))))
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
         auto gain = float(_comp(env)) * _makeup_gain;
         s = _clip(s * gain);
      }
      else
      {
         s = 0.0f;
         _is_note_on = false;
      }

      // Synthesize an envelope
      auto onset = _onset(std::abs(s));
      if (onset != 0.0f)
         _peak = onset;

      if (_y < _peak)
      {
         _is_note_on = true;
         _y = 1.6f + _attack * (_y - 1.6f);
         if (_y > _peak)
            _y = _peak + hysteresis;
      }
      else
      {
         _peak = 0;
         auto level = _onset._lp();
         _y = level + _decay * (_y - level);
         if (_y < level + hysteresis)
            _y = level;
      }

      return s;
   }
}}

#endif
