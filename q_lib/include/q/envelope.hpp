/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_ENVELOPE_HPP_MAY_17_2018)
#define CYCFI_Q_ENVELOPE_HPP_MAY_17_2018

#include <q/literals.hpp>
#include <q/sfx.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // envelope: Generates ADSR envelopes. attack_rate, decay_rate,
   // sustain_level, sustain_rate and release_rate determine the envelope
   // shape. The trigger member functions start the attack. The envelope can
   // be retriggered multiple times. The release member function starts the
   // release.
   ////////////////////////////////////////////////////////////////////////////
   class envelope
   {
   public:

      static constexpr float hysteresis = 0.0001; // -80dB

      enum state_enum
      {
         note_off_state       = 0
       , attack_state         = 6
       , legato_state         = 5
       , decay_state          = 4
       , sustain_state        = 3
       , release_state        = 2
       , note_release_state   = 1
      };

      struct config
      {
         // Default settings

         duration             attack_rate    = 30_ms;
         duration             decay_rate     = 70_ms;
         decibel              sustain_level  = -6_dB;
         duration             sustain_rate   = 50_s;
         duration             release_rate   = 100_ms;
      };

                              envelope(config const& config_, std::uint32_t sps);
                              envelope(std::uint32_t sps);

      float                   operator()();
      float                   current() const { return _y; }
      void                    trigger(float velocity, int auto_decay = 1);
      void                    legato();
      void                    decay();
      void                    release();
      state_enum              state() const;

      void                    attack_rate(duration rate, std::uint32_t sps);
      void                    decay_rate(duration rate, std::uint32_t sps);
      void                    sustain_level(float level);
      void                    sustain_rate(duration rate, std::uint32_t sps);
      void                    release_rate(duration rate, std::uint32_t sps);
      void                    release_rate(float rate);
      void                    note_off_level(float level);

      float                   velocity() const        { return _velocity; }
      float                   sustain_level() const   { return _sustain_level; }

   private:

      void                    update_legato();
      void                    update_attack();
      void                    update_decay();
      void                    update_sustain();
      void                    update_release();

      state_enum              _state = note_off_state;
      float                   _y = 0.0f;
      float                   _attack_rate;
      float                   _velocity;
      float                   _decay_rate;
      float                   _legato_level;
      float                   _sustain_level;
      float                   _start_sustain_level;
      float                   _sustain_rate;
      float                   _release_rate;
      float                   _note_off_level = 0.0f;
      float                   _end_note_off_level = 0.0f;
      int                     _auto_decay = 1;
   };

   ////////////////////////////////////////////////////////////////////////////
   // envelope_tracker
   ////////////////////////////////////////////////////////////////////////////
   struct envelope_tracker
   {
      struct config
      {
         // Onset detector
         double               onset_sensitivity    = 0.8;
         duration             onset_decay          = 100_ms;
         decibel              release_threshold    = -30_dB;

         // Noise gate
         duration             gate_release         = 30_ms;
         decibel              gate_on_threshold    = -36_dB;
         decibel              gate_off_threshold   = -60_dB;

         // Compressor
         decibel              comp_threshold       = -18_dB;
         double               comp_slope           = 1.0/20;
         double               comp_gain            = 4;

         // Attack Variance
         decibel              attack_variance      = 2_dB;
      };

                              envelope_tracker(std::uint32_t sps);
                              envelope_tracker(config const& conf, std::uint32_t sps);
      float                   operator()(float s, envelope& env_gen);

      onset_detector          _onset;
      peak_envelope_follower  _env;
      compressor              _comp;
      window_comparator       _gate;
      float                   _release_threshold;
      float                   _end_release;
      float                   _makeup_gain;
      float                   _attack_variance;
      float                   _attack_min;
      float                   _attack_max;
      float                   _peak_attack = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline envelope::envelope(config const& config_, std::uint32_t sps)
    : _attack_rate(fast_exp3(-2.0f / (sps * double(config_.attack_rate))))
    , _decay_rate(fast_exp3(-2.0f / (sps * double(config_.decay_rate))))
    , _sustain_level(float(config_.sustain_level))
    , _sustain_rate(fast_exp3(-2.0f / (sps * double(config_.sustain_rate))))
    , _release_rate(fast_exp3(-2.0f / (sps * double(config_.release_rate))))
   {}

   inline envelope::envelope(std::uint32_t sps)
    : envelope(config{}, sps)
   {}

   inline void envelope::attack_rate(duration rate, std::uint32_t sps)
   {
      _attack_rate = fast_exp3(-2.0f / (sps * double(rate)));
   }

   inline void envelope::decay_rate(duration rate, std::uint32_t sps)
   {
      _decay_rate = fast_exp3(-2.0f / (sps * double(rate)));
   }

   inline void envelope::sustain_level(float level)
   {
      _sustain_level = level;
   }

   inline void envelope::sustain_rate(duration rate, std::uint32_t sps)
   {
      _sustain_rate = fast_exp3(-2.0f / (sps * double(rate)));
   }

   inline void envelope::release_rate(duration rate, std::uint32_t sps)
   {
      _release_rate = fast_exp3(-2.0f / (sps * double(rate)));
   }

   inline void envelope::release_rate(float rate)
   {
      if (rate < 1.0f)
         _release_rate = rate;
   }

   inline void envelope::note_off_level(float level)
   {
      if (level < _y)
      {
         _note_off_level = level;
         _end_note_off_level = level * 0.1;
      }
   }

   inline float envelope::operator()()
   {
      switch (_state)
      {
         case note_off_state:
            return 0.0f;

         case legato_state:
            update_legato();
            break;

         case attack_state:
            update_attack();
            break;

         case decay_state:
            update_decay();
            break;

         case sustain_state:
            update_sustain();
            break;

         case note_release_state:
         case release_state:
            update_release();
            break;
      }

      return _y;
   }

   inline void envelope::trigger(float velocity, int auto_decay)
   {
      if (_y < velocity)
      {
         _auto_decay = auto_decay;
         _velocity = velocity;
         _state = attack_state;
      }
   }

   inline void envelope::legato()
   {
      if (_state == sustain_state && _y < _start_sustain_level)
      {
         _auto_decay = -1;
         _legato_level = _start_sustain_level;
         _state = legato_state;
      }
   }

   inline void envelope::decay()
   {
      _auto_decay = 1; // auto decay after attack
   }

   inline void envelope::update_legato()
   {
      _y = _legato_level + _attack_rate * (_y - _legato_level);
      if (_y < _legato_level + hysteresis)
      {
         _y = _legato_level;
         _state = sustain_state;
      }
   }

   inline void envelope::update_attack()
   {
      _y = 1.6f + _attack_rate * (_y - 1.6f);
      if (_y > _velocity)
      {
         _y = _velocity;
         switch (_auto_decay)
         {
            case 1: _state = decay_state; break;
            case -1: _state = sustain_state; break;
         }
      }
   }

   inline void envelope::update_decay()
   {
      auto level = _velocity * _sustain_level;
      _y = level + _decay_rate * (_y - level);
      if (_y < level + hysteresis)
      {
         _y = level;
         _start_sustain_level = level;
         _state = sustain_state;
      }
   }

   inline void envelope::update_sustain()
   {
      _y *= _sustain_rate;
   }

   inline void envelope::release()
   {
      if (_state != note_off_state)
         _state = release_state;
   }

   inline void envelope::update_release()
   {
      _y = _note_off_level + _release_rate * (_y - _note_off_level);
      if (_y < _note_off_level + hysteresis)
      {
         _y = _note_off_level;
         if (_y < _end_note_off_level)
            _state = note_release_state;
         else if (_y < hysteresis)
            _state = note_off_state;
      }
   }

   inline envelope::state_enum envelope::state() const
   {
      return _state;
   }

   inline envelope_tracker::envelope_tracker(config const& conf, std::uint32_t sps)
    : _onset(conf.onset_sensitivity, conf.onset_decay, sps)
    , _env(conf.gate_release, sps)
    , _comp(conf.comp_threshold, conf.comp_slope)
    , _gate(float(conf.gate_off_threshold), float(conf.gate_on_threshold))
    , _release_threshold(conf.release_threshold)
    , _makeup_gain(conf.comp_gain)
    , _attack_variance(float(conf.attack_variance))
   {}

   inline envelope_tracker::envelope_tracker(std::uint32_t sps)
    : envelope_tracker(config{}, sps)
   {}

   inline float envelope_tracker::operator()(float s, envelope& env_gen)
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
      }

      // Attack
      auto onset = _onset(s);

      // Update generated envelope
      if (onset != 0.0f)
      {
         if (_peak_attack != 0.0f)
         {
            if (onset < _attack_min)
               onset = _attack_min;
            else if (onset > _attack_max)
               onset = _attack_max;
         }

         env_gen.trigger(onset, true); // trigger, no auto decay
      }
      else
      {
         if (env_gen.state() != envelope::note_off_state)
         {
            if (env_gen.state() == envelope::attack_state)
            {
               if (env_gen.velocity() > _peak_attack)
               {
                  _peak_attack = env_gen.velocity();
                  _attack_max = _attack_variance * _peak_attack;
                  _attack_min = fast_inverse(_attack_variance) * _peak_attack;
               }
            }

            if (_onset._lp() < env_gen.velocity() * _release_threshold)
            {
               // release
               env_gen.release();

               // Make the release envelope follow the input envelope
               env_gen.note_off_level(_onset._lp());
            }
         }
      }

      return s;
   }
}}

#endif
