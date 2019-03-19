/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_ALL_PASS_DECEMBER_7_2018)
#define CYCFI_Q_ALL_PASS_DECEMBER_7_2018

#include <q/support/base.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // Basic one-pole allpass filter
   ////////////////////////////////////////////////////////////////////////////
   struct one_pole_allpass
   {
      // a: location of the pole in the range -1..1
      one_pole_allpass(float a)
       : a(a)
      {}

      static float coef(frequency freq, std::uint32_t sps)
      {
         auto w = pi * double(freq) / sps;
         return fasttan(w - 0.25_pi);
      }

      // 90 deg shift at frequency f
      one_pole_allpass(frequency freq, std::uint32_t sps)
       : a(coef(freq, sps))
      {}

      float operator()(float s)
      {
         y = x + a * s;
         x = s - a * y;
         return y;
      }

      // set pivot (90 deg shift at frequency f)
      void pivot(frequency freq, std::uint32_t sps)
      {
         a = coef(freq, sps);
      }

      float y = 0.0f, x = 0.0f, a;
   };

   ////////////////////////////////////////////////////////////////////////////
   // 2-Pole polyphase IIR allpass filter (See http://yehar.com/blog/?p=368)
   ////////////////////////////////////////////////////////////////////////////
   struct polyphase_allpass
   {
      polyphase_allpass(float a)
       : a(a)
       , x1(0), x2(0), y1(0), y2(0)
      {}

      float operator()(float s)
      {
         auto r = a * (s + y2) - x2;

         // shift x1 to x2, s to x1
         x2 = x1;
         x1 = s;

         // shift y1 to y2, r to y1
         y2 = y1;
         y1 = r;

         return r;
      }

      float a;
      float x1, x2, y1, y2;
   };
}}

#endif
