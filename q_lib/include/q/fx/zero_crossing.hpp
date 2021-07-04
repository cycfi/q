/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_ZERO_CROSSING_DECEMBER_7_2018)
#define CYCFI_Q_ZERO_CROSSING_DECEMBER_7_2018

#include <q/support/base.hpp>
#include <q/support/decibel.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // zero_crossing generates pulses that coincide with the zero crossings of
   // the signal. To minimize noise, 1) we apply some amount of hysteresis
   // and 2) constrain the time between transitions to a minumum given
   // min_period (or max_freq).
   //
   // Note: the result is a bool.
   ////////////////////////////////////////////////////////////////////////////
   struct zero_crossing
   {
      zero_crossing(float hysteresis)
       : _hysteresis(-hysteresis)
      {}

      zero_crossing(decibel hysteresis)
       : _hysteresis(-as_float(hysteresis))
      {}

      bool operator()(float s)
      {
         // Offset s by half of hysteresis, so that zero cross detection is
         // centered on the actual zero.
         s += _hysteresis / 2;

         if (!_state && s > 0.0f)
            _state = 1;
         else if (_state && s < _hysteresis)
            _state = 0;
         return _state;
      }

      bool operator()() const
      {
         return _state;
      }

      float _hysteresis;
      bool _state = 0;
   };
}

#endif
