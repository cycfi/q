/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_ENVELOPE_HPP_MAY_17_2018)
#define CYCFI_Q_ENVELOPE_HPP_MAY_17_2018

#include <q/literals.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // The envelope follower will follow the envelope of a signal with gradual
   // release (given by the release parameter). The signal decays
   // exponentially if the signal is below the peak.
   //
   //    y:          current value
   //    _attack:    attack
   //    _release:   release
   //
   ////////////////////////////////////////////////////////////////////////////
   struct envelope_follower
   {
      envelope_follower(duration attack, duration release, std::uint32_t sps)
       : _attack(fast_exp3(-2.0f / (sps * double(attack))))
       , _release(fast_exp3(-2.0f / (sps * double(release))))
      {}

      float operator()(float s)
      {
         s = std::abs(s);
         return y = s + ((s > y)? _attack : _release) * (y - s);
      }

      float operator()() const
      {
         return y;
      }

      envelope_follower& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      void config(duration attack, duration release, std::uint32_t sps)
      {
         _attack = fast_exp3(-2.0f / (sps * double(attack)));
         _release = fast_exp3(-2.0f / (sps * double(release)));
      }

      void attack(float attack_, std::uint32_t sps)
      {
         _attack = fast_exp3(-2.0f / (sps * attack_));
      }

      void release(float release_, std::uint32_t sps)
      {
         _release = fast_exp3(-2.0f / (sps * release_));
      }

      float y = 0.0f, _attack, _release;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Same as envelope follower above, but with attack = 0;
   //
   //    y:          current value
   //    _release:   release
   //
   ////////////////////////////////////////////////////////////////////////////
   struct peak_envelope_follower
   {
      peak_envelope_follower(duration release, std::uint32_t sps)
       : _release(fast_exp3(-2.0f / (sps * double(release))))
      {}

      float operator()(float s)
      {
         if (s > y)
            y = s;
         else
            y = s + _release * (y - s);
         return y;
      }

      float operator()() const
      {
         return y;
      }

      peak_envelope_follower& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      void release(float release_, std::uint32_t sps)
      {
         _release = fast_exp3(-2.0f / (sps * release_));
      }

      float y = 0.0f, _release;
   };

   ////////////////////////////////////////////////////////////////////////////
   // This envelope follower combines fast response, low ripple.
   //
   // Based on http://tinyurl.com/yat2tuf8
   //
   // There is no filtering. The output is jagged, staircase like. That way,
   // this can be useful for analysis such as onset detection.
   ////////////////////////////////////////////////////////////////////////////
   struct fast_envelope_follower
   {
      fast_envelope_follower(duration hold, std::uint32_t sps)
       : _window(float(0.5_ms) * sps)
       , _reset((float(hold) * sps) / _window)
      {}

      float operator()(float s)
      {
         // Do this every 0.5ms (window), collecting the peak in the meantime
         if (_i2++ != _window)
         {
            if (s > _peak)
               _peak = s;
            return _latest;
         }

         // This part of the code gets called every 0.5ms (window)
         // Get the peak and hold it in _y1 and _y2
         _i2 = 0;
         if (_peak > _y1)
            _y1 = _peak;
         if (_peak > _y2)
            _y2 = _peak;

         // Reset _y1 and _y2 alternately every so often (the hold parameter)
         if (_tick++ == _reset)
         {
            _tick = 0;
            if (_i++ & 1)
               _y1 = 0;
            else
               _y2 = 0;
         }

         // The peak is the maximum of _y1 and _y2
         _latest = std::max(_y1, _y2);
         _peak = 0;
         return _latest;
      }

      float _y1 = 0, _y2 = 0, _peak = 0, _latest = 0;
      std::uint16_t _tick = 0, _i = 0, _i2 = 0;
      std::uint16_t const _window, _reset;
   };

   ////////////////////////////////////////////////////////////////////////////
   struct envelope_shaper
   {
      static constexpr float hysteresis = 0.0001; // -80dB

      envelope_shaper(duration attack, duration decay, std::uint32_t sps)
       : _attack(fast_exp3(-2.0f / (sps * double(attack))))
       , _decay(fast_exp3(-2.0f / (sps * double(decay))))
      {}

      float operator()(float s)
      {
         if (y < _peak || s > y) // upward
         {
            if (_peak < s)
               _peak = s;
            y = 1.6f + _attack * (y - 1.6f);
            if (y > _peak)
               _peak = 0;
         }
         else
         {
            y = s + _decay * (y - s);
            if (y < hysteresis)
               _peak = y = 0;
         }
         return y;
      }

      void config(duration attack, duration decay, std::uint32_t sps)
      {
         _attack = fast_exp3(-2.0f / (sps * double(attack)));
         _decay = fast_exp3(-2.0f / (sps * double(decay)));
      }

      void attack(float attack_, std::uint32_t sps)
      {
         _attack = fast_exp3(-2.0f / (sps * attack_));
      }

      void release(float release_, std::uint32_t sps)
      {
         _decay = fast_exp3(-2.0f / (sps * release_));
      }

      float y = 0, _peak = 0, _attack, _decay;
   };

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
   // envelope_processor
   ////////////////////////////////////////////////////////////////////////////
   class envelope_processor
   {
   public:

      static constexpr float hysteresis = 0.0001; // -80dB

      struct config
      {
         // Envelope Follower
         duration             env_hold             = 10_ms;

         // Compressor
         duration             comp_release         = 30_ms;
         decibel              comp_threshold       = -18_dB;
         decibel              comp_width           = 3_dB;
         double               comp_slope           = 1.0/4;
         double               comp_gain            = 4;

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
      float                   envelope() const     { return _synth_env_val; }
      bool                    is_note_on() const   { return _is_note_on; }

   private:

      peak_envelope_follower  _env;
      fast_envelope_follower  _fast_env;
      envelope_shaper         _synth_env;
      soft_knee_compressor    _comp;
      window_comparator       _gate;

      float                   _makeup_gain;
      bool                    _is_note_on = false;
      float                   _synth_env_val;
   };

   ////////////////////////////////////////////////////////////////////////////
   // envelope implementation
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

   ////////////////////////////////////////////////////////////////////////////
   // envelope_processor implementation
   ////////////////////////////////////////////////////////////////////////////
   inline envelope_processor::envelope_processor(config const& conf, std::uint32_t sps)
    : _env(conf.comp_release, sps)
    , _fast_env(conf.env_hold, sps)
    , _synth_env(conf.attack, conf.decay, sps)
    , _comp(conf.comp_threshold, conf.comp_width, conf.comp_slope)
    , _gate(float(conf.gate_off_threshold), float(conf.gate_on_threshold))
    , _makeup_gain(conf.comp_gain)
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
         _is_note_on = true;

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
      auto synth_env = _fast_env(std::abs(s));
      _synth_env_val = _synth_env(synth_env);

      return s;
   }
}}

#endif
