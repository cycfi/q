/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SCHMITT_TRIGGER_DECEMBER_7_2018)
#define CYCFI_Q_SCHMITT_TRIGGER_DECEMBER_7_2018

#include <q/support/base.hpp>
#include <q/support/decibel.hpp>
#include <q/fx/envelope.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // The output of a simple comparator is determined by its inputs. The
   // output is `1` if the input signal is greater than the reference signal
   // plus a specified hysteresis. The output is `0` if the input signal is
   // less than the reference signal minus the specified hysteresis.
   // Otherwise, the previous result is retained.
   //
   // Note: the result is a bool.
   ////////////////////////////////////////////////////////////////////////////
   struct schmitt_trigger
   {
      schmitt_trigger(float hysteresis)
       : _hysteresis(hysteresis)
      {}

      schmitt_trigger(decibel hysteresis)
       : _hysteresis(lin_float(hysteresis))
      {}

      bool operator()(float s, float ref)
      {
         if (!y && s > (ref + _hysteresis))
            y = 1;
         else if (y && s < (ref - _hysteresis))
            y = 0;
         return y;
      }

      bool operator()() const
      {
         return y;
      }

      float const _hysteresis;
      bool        y = 0;
   };
}

#endif
