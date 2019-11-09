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
       , duration release
       , decibel release_threshold
       , std::uint32_t sps
      ) : envelope_shaper(
         fast_exp3(-2.0f / (sps * double(attack)))
       , fast_exp3(-2.0f / (sps * double(decay)))
       , fast_exp3(-2.0f / (sps * double(release)))
       , float(release_threshold))
      {}

      envelope_shaper(
         float attack
       , float decay
       , float release
       , float release_threshold
      )
       : _attack(attack)
       , _decay(decay)
       , _release(release)
       , _release_threshold(release_threshold)
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
         else
         {
            auto threshold = _release_threshold * _hold;
            auto slope = (y < threshold)? _release : _decay;
            y = slope * y;
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
      float _attack, _decay, _release, _release_threshold;
   };
}}

#endif
