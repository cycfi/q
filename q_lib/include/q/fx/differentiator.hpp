/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DIFFERENTIATOR_HPP_DECEMBER_24_2015)
#define CYCFI_Q_DIFFERENTIATOR_HPP_DECEMBER_24_2015

#include <q/fx/delay.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // The differentiator returns the time derivative of the input (s).
   ////////////////////////////////////////////////////////////////////////////
   struct differentiator
   {
      differentiator()
       : x(0.0f) {}

      float operator()(float s)
      {
         auto val = s - x;
         x = s;
         return val;
      }

      float x; // delayed input sample
   };

   ////////////////////////////////////////////////////////////////////////////
   // central_difference is a differentiator with this time-domain expression:
   //
   //    y(n) = (x(n) - x(n-2)) / 2
   //
   // Unlike first-difference differentiator (see differentiator),
   // central_difference has better immunity to high-frequency noise. See
   // https://www.dsprelated.com/showarticle/35.php
   ////////////////////////////////////////////////////////////////////////////
   struct central_difference
   {
      float operator()(float s)
      {
         return (s - _d(s)) / 2;
      }

      delay2 _d;
   };
}

#endif
