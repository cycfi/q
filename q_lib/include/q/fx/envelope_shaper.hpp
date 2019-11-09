/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FX_ENVELOPE_SHAPER_HPP_MAY_17_2018)
#define CYCFI_Q_FX_ENVELOPE_SHAPER_HPP_MAY_17_2018

#include <q/support/literals.hpp>
#include <algorithm>

namespace cycfi { namespace q
{
   using namespace q::literals;

   ////////////////////////////////////////////////////////////////////////////
   // envelope_shaper is an envelope processor that allows control of the
   // envelope's attack, decay and release parameters. Take note that the
   // envelope_shaper is a processor and does not synthesize an envelope.
   ////////////////////////////////////////////////////////////////////////////
   struct envelope_shaper
   {
      static constexpr auto silence = float(-80_dB);

      envelope_shaper(
         duration attack
       , duration decay
       , decibel sustain_level
       , duration sustain_rate
       , decibel release_level
       , duration release
       , std::uint32_t sps
      ) : envelope_shaper(
         fast_exp3(-2.0f / (sps * double(attack)))
       , fast_exp3(-2.0f / (sps * double(decay)))
       , float(sustain_level)
       , fast_exp3(-2.0f / (sps * double(sustain_rate)))
       , float(release_level)
       , fast_exp3(-2.0f / (sps * double(release)))
      )
      {}

      envelope_shaper(
         float attack
       , float decay
       , float sustain_level
       , float sustain_rate
       , float release_level
       , float release
      )
       : _attack(attack)
       , _decay(decay)
       , _sustain_level(sustain_level)
       , _sustain_rate(sustain_rate)
       , _release_level(release_level)
       , _release(release)
      {}

      float operator()(float s)
      {
         if (y < _peak || s > 0) // attack or legato
         {
            if (_peak < s)
               _peak = s;
            auto target = 1.6f * _peak;
            y = target + _attack * (y - target);
            if (y > _peak)
            {
               _hold = _peak;
               _peak = 0;
            }
         }
         else if (y < silence)
         {
            _peak = y = 0;
         }
         else if (y < _release_level)
         {
            y *= _release;
         }
         else if (y < _sustain_level)
         {
            y *= _sustain_rate;
         }
         else
         {
            auto level = _hold * _sustain_level;
            y = level + _decay * (y - level);
         }
         return y;
      }

      float operator()() const
      {
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

      float y = 0, _peak = 0, _hold = 0;
      float _attack, _decay, _sustain_level, _sustain_rate;
      float _release_level, _release;
   };
}}

#endif
