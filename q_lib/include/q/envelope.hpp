/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_ENVELOPE_HPP_MAY_17_2018)
#define CYCFI_Q_ENVELOPE_HPP_MAY_17_2018

#include <q/literals.hpp>

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

      static constexpr float hysteresis = 0.001; // -60dB

      enum state_enum
      {
         note_off_state = 0
       , attack_state   = 4
       , decay_state    = 3
       , sustain_state  = 2
       , release_state  = 1
      };

                     envelope(
                        duration attack_rate
                      , duration decay_rate
                      , float sustain_level
                      , duration sustain_rate
                      , duration release_rate
                      , std::uint32_t sps
                     );

      float          operator()();
      void           trigger(float attack_level);
      void           trigger();
      void           release();
      state_enum     state() const;

      void           attack_rate(float rate, std::uint32_t sps);
      void           decay_rate(float rate, std::uint32_t sps);
      void           sustain_level(float level);
      void           sustain_rate(float rate, std::uint32_t sps);
      void           release_rate(float rate, std::uint32_t sps);

   private:

      void           update_attack();
      void           update_decay();
      void           update_sustain();
      void           update_release();

      state_enum     _state = note_off_state;
      float          _y = 0.0f;
      float          _attack_rate;
      float          _attack_level = 1.0f;
      float          _decay_rate;
      float          _sustain_level;
      float          _sustain_rate;
      float          _release_rate;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline envelope::envelope(
      duration attack_rate
    , duration decay_rate
    , float sustain_level
    , duration sustain_rate
    , duration release_rate
    , std::uint32_t sps
   )
    : _attack_rate(std::exp(-2.0f / (sps * double(attack_rate))))
    , _decay_rate(std::exp(-2.0f / (sps * double(decay_rate))))
    , _sustain_level(sustain_level)
    , _sustain_rate(std::exp(-2.0f / (sps * double(sustain_rate))))
    , _release_rate(std::exp(-2.0f / (sps * double(release_rate))))
   {}

   inline void envelope::attack_rate(float rate, std::uint32_t sps)
   {
      _attack_rate = std::exp(-2.0f / (sps * double(rate)));
   }

   inline void envelope::decay_rate(float rate, std::uint32_t sps)
   {
      _decay_rate = std::exp(-2.0f / (sps * double(rate)));
   }

   inline void envelope::sustain_level(float level)
   {
      _sustain_level = level;
   }

   inline void envelope::sustain_rate(float rate, std::uint32_t sps)
   {
      _sustain_rate = std::exp(-2.0f / (sps * double(rate)));
   }

   inline void envelope::release_rate(float rate, std::uint32_t sps)
   {
      _release_rate = std::exp(-2.0f / (sps * double(rate)));
   }

   inline float envelope::operator()()
   {
      switch (_state)
      {
         case note_off_state:
            return 0.0f;

         case attack_state:
            update_attack();
            break;

         case decay_state:
            update_decay();
            break;

         case sustain_state:
            update_sustain();
            break;

         case release_state:
            update_release();
            break;
      }

      return _y;
   }

   inline void envelope::trigger(float attack_level)
   {
      if (_state != attack_state)
      {
         _attack_level = attack_level;
         _state = attack_state;
      }
   }

   inline void envelope::trigger()
   {
      trigger(_sustain_level);
   }

   inline void envelope::update_attack()
   {
      _y = 1.6f + _attack_rate * (_y - 1.6f);
      if (_y > _attack_level)
      {
         _y = _attack_level;
         _state = decay_state;
      }
   }

   inline void envelope::update_decay()
   {
      _y = _sustain_level + _decay_rate * (_y - _sustain_level);
      if (_y < _sustain_level + hysteresis)
      {
         _y = _sustain_level;
         _state = sustain_state;
      }
   }

   inline void envelope::update_sustain()
   {
      _y *= _sustain_rate;
   }

   inline void envelope::release()
   {
      _state = release_state;
   }

   inline void envelope::update_release()
   {
      _y *= _release_rate;
      if (_y < hysteresis)
      {
         _y = 0.0f;
         _state = note_off_state;
      }
   }

   inline envelope::state_enum envelope::state() const
   {
      return _state;
   }
}}

#endif
