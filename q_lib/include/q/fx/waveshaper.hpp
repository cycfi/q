/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_WAVE_SHAPER_DECEMBER_24_2015)
#define CYCFI_Q_WAVE_SHAPER_DECEMBER_24_2015

#include <q/support/base.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // clip a signal to range -_max...+_max
   //
   //    max: maximum value
   ////////////////////////////////////////////////////////////////////////////
   struct clip
   {
      constexpr clip(float max = 1.0f)
       : _max(max)
      {}

      constexpr float operator()(float s) const
      {
         return (s > _max) ? _max : (s < -_max) ? -_max : s;
      }

      float _max;
   };

   ////////////////////////////////////////////////////////////////////////////
   // soft_clip a signal to range -1.0 to 1.0.
   ////////////////////////////////////////////////////////////////////////////
   struct soft_clip : clip
   {
      constexpr float operator()(float s) const
      {
         s = clip::operator()(s);
         return 1.5 * s - 0.5 * s * s * s;
      }
   };
}

#endif
