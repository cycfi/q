/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_ANTIALIASING_HPP_MAY_19_2018)
#define CYCFI_Q_ANTIALIASING_HPP_MAY_19_2018

#include <q/phase.hpp>

namespace cycfi { namespace q { namespace detail
{
   constexpr float poly_blep(phase p, phase dt)
   {
      constexpr auto end = phase::end();
      constexpr auto one_cyc = phase::one_cyc;

      if (p < dt)
      {
         auto t = float(p.val) / dt.val;
         return t+t - t*t - 1.0f;
      }
      else if (p > end - dt)
      {
         auto t = -float((end - p).val) / dt.val;
         return t*t + t+t + 1.0f;
      }
      else
      {
         return 0.0f;
      }
   }

   constexpr double poly_blamp(phase p, phase dt, float scale)
   {
      constexpr auto end = phase::end();

      if (p < dt)
      {
         auto t = (float(p.val) / dt.val) - 1.0f;
         return scale * float(dt) * -1.0f/3 * t*t*t;
      }
      else if (p > end - dt)
      {
         auto t = -(float((end - p).val) / dt.val) + 1.0f;
         return scale * float(dt) * 1.0f/3 * t*t*t;
      }
      else
      {
         return 0.0f;
      }
   }
}}}

#endif
