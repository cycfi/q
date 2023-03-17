/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_LEVEL_CROSSFADE_HPP_DECEMBER_24_2015)
#define CYCFI_Q_LEVEL_CROSSFADE_HPP_DECEMBER_24_2015

#include <q/support/literals.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // level_crossfade smoothly fades-in and fades-out two signals, `a` and
   // `b`, when a control argument, `ctrl`, falls below a given `pivot`. If
   // `ctrl` is above the pivot (e.g. -10dB) the gain of `a` is 1.0 and the
   // gain of `b` is 0.0. if `ctrl` falls below the pivot (e.g. -10dB), `a`
   // fades-out while `b` fades-in smoothly by (ctrl - pivot) decibels.
   //
   // For example, if `pivot` is -10dB, and `ctrl` is -13dB, the gain of `a`
   // is 0.708 (-3dB == -10dB - -13dB) and the gain of `b` is 0.3 (1.0 -
   // 0.708).
   ////////////////////////////////////////////////////////////////////////////
   struct level_crossfade
   {
      constexpr level_crossfade(decibel pivot)
       : _pivot(pivot)
      {}

      float operator()(float a, float b, decibel ctrl)
      {
         if (ctrl < _pivot)
         {
            auto xfade = as_float(ctrl - _pivot);
            return xfade * a + (1.0 - xfade) * b;
         }
         return a;
      }

      void pivot(decibel pivot_)
      {
         _pivot = pivot_;
      }

      decibel  _pivot;
   };
}

#endif
