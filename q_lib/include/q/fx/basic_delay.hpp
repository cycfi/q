/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DELAY_DECEMBER_24_2015)
#define CYCFI_Q_DELAY_DECEMBER_24_2015

#include <q/support.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // Basic one unit delay
   ////////////////////////////////////////////////////////////////////////////
   struct delay1
   {
      float operator()(float s)
      {
         auto r = y;
         y = s;
         return r;
      }

      float operator()() const
      {
         return y;
      }

      float y = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Basic two unit delay
   ////////////////////////////////////////////////////////////////////////////
   struct delay2
   {
      float operator()(float s)
      {
         return _d2(_d1(s));
      }

      delay1 _d1, _d2;
   };
}}

#endif
