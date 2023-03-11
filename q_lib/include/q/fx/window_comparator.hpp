/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_WINDOW_COMPARATOR_DECEMBER_7_2018)
#define CYCFI_Q_WINDOW_COMPARATOR_DECEMBER_7_2018

#include <q/support/base.hpp>
#include <q/support/decibel.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // window_comparator. If input (s) exceeds a high threshold (_high), the
   // current state (y) becomes 1. Else, if input (s) is below a low
   // threshold (_low), the current state (y) becomes 0. If the state (s)
   // is in between the low and high thresholds, the previous state is kept.
   //
   //    low:     low threshold
   //    high:    high threshold
   //
   // Note: the result is a bool.
   ////////////////////////////////////////////////////////////////////////////
   struct window_comparator
   {
      window_comparator(float low, float high)
       : _low(low), _high(high)
      {}

      window_comparator(decibel low, decibel high)
       : _low(as_float(low)), _high(as_float(high))
      {}

      bool operator()(float s)
      {
         if (s < _low)
            y = 0;
         else if (s > _high)
            y = 1;
         return y;
      }

      bool operator()() const
      {
         return y;
      }

      window_comparator& operator=(bool y_)
      {
         y = y_;
         return *this;
      }

      void threshold(float low, float high)
      {
         _low = low;
         _high = high;
      }

      void threshold(decibel low, decibel high)
      {
         _low = as_float(low);
         _high = as_float(high);
      }

      float _low, _high;
      bool y = 0;
   };
}

#endif
