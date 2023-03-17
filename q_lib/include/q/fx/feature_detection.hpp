/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SCHMITT_TRIGGER_DECEMBER_7_2018)
#define CYCFI_Q_SCHMITT_TRIGGER_DECEMBER_7_2018

#include <q/support/base.hpp>
#include <q/support/decibel.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // The output of a simple comparator is determined by its inputs. The
   // output is high (1) if the positive input (pos) is greater than the
   // negative input (neg). Otherwise, the output is low (0).
   //
   // The schmitt trigger adds some hysteresis to improve noise immunity and
   // minimize multiple triggering by adding and subtracting a certain
   // fraction back to the negative input (neg). Hysteresis is the fraction
   // (should be less than < 1.0) that determines how much is added or
   // subtracted. By doing so, the comparator "bar" is raised or lowered
   // depending on the previous state.
   //
   // Note: the result is a bool.
   ////////////////////////////////////////////////////////////////////////////
   struct schmitt_trigger
   {
      schmitt_trigger(float hysteresis)
       : _hysteresis(hysteresis)
      {}

      schmitt_trigger(decibel hysteresis)
       : _hysteresis(as_float(hysteresis))
      {}

      bool operator()(float pos, float neg)
      {
         if (!y && pos > (neg + _hysteresis))
            y = 1;
         else if (y && pos < (neg - _hysteresis))
            y = 0;
         return y;
      }

      bool operator()() const
      {
         return y;
      }

      float const _hysteresis;
      bool y = 0;
   };
}

#endif
