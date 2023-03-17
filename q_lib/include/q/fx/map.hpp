/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MAP_HPP_DECEMBER_24_2015)
#define CYCFI_Q_MAP_HPP_DECEMBER_24_2015

#include <q/support/base.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Map the input, s (with expected value 0 to 1) to y1 to y2 using linear
   // interpolation. Example: if the range (y1, y2) is (0.5, 0.8), the input
   // value 0 is mapped to 0.5 while the value 1 is mapped to 0.8.
   //
   // Note that y1 can be greater than y2. Example: if the range (y1, y2) is
   // (1.0, 0.0), the input value 0 is mapped to 1 while the value 1 is
   // mapped to 0.
   ////////////////////////////////////////////////////////////////////////////
   struct map
   {
      constexpr map(float y1, float y2)
       : _y1(y1)
       , _y2(y2)
      {}

      constexpr float operator()(float s) const
      {
         return linear_interpolate(_y1, _y2, s);
      }

      void range(float y1, float y2)
      {
         _y1 = y1;
         _y2 = y2;
      }

      float _y1, _y2;
   };
}

#endif
