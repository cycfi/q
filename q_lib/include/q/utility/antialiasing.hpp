/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_ANTIALIASING_HPP_MAY_19_2018)
#define CYCFI_Q_ANTIALIASING_HPP_MAY_19_2018

#include <q/support/phase.hpp>

namespace cycfi::q
{
   constexpr float poly_blep(phase p, phase dt)
   {
      constexpr auto end = phase::max();

      if (p < dt)
      {
         auto t = float(p.rep) / dt.rep;
         return t+t - t*t - 1.0f;
      }
      else if (p > end - dt)
      {
         auto t = -float((end - p).rep) / dt.rep;
         return t*t + t+t + 1.0f;
      }
      else
      {
         return 0.0f;
      }
   }

   constexpr double poly_blamp(phase p, phase dt, float scale)
   {
      constexpr auto end = phase::max();

      if (p < dt)
      {
         auto t = (float(p.rep) / dt.rep) - 1.0f;
         return -scale/3 * float(dt) * t*t*t;
      }
      else if (p > end - dt)
      {
         auto t = -(float((end - p).rep) / dt.rep) + 1.0f;
         return scale/3 * float(dt) * t*t*t;
      }
      else
      {
         return 0.0f;
      }
   }
}

#endif
