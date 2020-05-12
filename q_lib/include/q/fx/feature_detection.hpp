/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FEATURE_DETECTION_DECEMBER_7_2018)
#define CYCFI_Q_FEATURE_DETECTION_DECEMBER_7_2018

#include <q/support/base.hpp>
#include <q/support/decibel.hpp>
#include <q/fx/envelope.hpp>

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
       : _hysteresis(float(hysteresis))
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
      bool        y = 0;
   };

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
       : _low(float(low)), _high(float(high))
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

      float _low, _high;
      bool y = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // zero_cross generates pulses that coincide with the zero crossings
   // of the signal. To minimize noise, 1) we apply some amount of hysteresis
   // and 2) constrain the time between transitions to a minumum given
   // min_period (or max_freq).
   //
   // Note: the result is a bool.
   ////////////////////////////////////////////////////////////////////////////
   struct zero_cross
   {
      zero_cross(float hysteresis)
       : _cmp(hysteresis)
      {}

      zero_cross(decibel hysteresis)
       : _cmp(float(hysteresis))
      {}

      bool operator()(float s)
      {
         return _cmp(s, 0);
      }

      schmitt_trigger   _cmp;
      bool              _state = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // peak generates pulses that coincide with the peaks of a waveform. This
   // is accomplished by comparing the signal with the (slightly attenuated)
   // envelope of the signal (env) using a schmitt_trigger.
   //
   //    sensitivity: Envelope droop amount (attenuation)
   //    hysteresis: schmitt_trigger hysteresis amount
   //
   // The result is a bool corresponding to the peaks.
   ////////////////////////////////////////////////////////////////////////////
   struct peak
   {
      peak(float sensitivity, float hysteresis)
       : _sensitivity(sensitivity), _cmp(hysteresis)
      {}

      peak(float sensitivity, decibel hysteresis)
       : _sensitivity(sensitivity), _cmp(float(hysteresis))
      {}

      bool operator()(float s, float env)
      {
         return _cmp(s, env * _sensitivity);
      }

      bool operator()() const
      {
         return _cmp();
      }

      float const       _sensitivity;
      schmitt_trigger   _cmp;
   };
}

#endif
