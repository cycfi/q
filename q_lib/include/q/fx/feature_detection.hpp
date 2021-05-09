/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

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

   ////////////////////////////////////////////////////////////////////////////
   // zero_cross generates pulses that coincide with the zero crossings
   // of the signal. To minimize noise, 1) we apply some amount of hysteresis
   // and 2) constrain the time between transitions to a minumum given
   // min_period (or max_freq).
   //
   // Note: the result is a bool.
   ////////////////////////////////////////////////////////////////////////////
   struct zero_crossing
   {
      constexpr static auto threshold1 = 0.2f;
      constexpr static auto threshold2 = 0.5f;

      zero_crossing(float hysteresis)
       : _hysteresis(-hysteresis)
       , _state0{0}
       , _state1{0}
       , _state2{0}
      {}

      zero_crossing(decibel hysteresis)
       : _hysteresis(-as_float(hysteresis))
       , _state0{0}
       , _state1{0}
       , _state2{0}
      {}

      static constexpr bool discriminate(
         float threshold, float slope, float& slope_env
      )
      {
         if (slope > (slope_env * threshold * 0.5))
         {
            slope_env *= threshold;
            if (slope > slope_env)
            {
               slope_env = slope;
               return 1;
            }
         }
         return 0;
      }

      bool operator()(float s, bool gate)
      {
         if (!gate)
         {
            if (_state0 || _state1 || _state2)
               reset();
            return 0; // return early
         }

         // Offset s by half of hysteresis, so that zero cross detection is
         // centered on the actual zero.
         s += _hysteresis / 2;

         auto prev_state = _state0;
         if (!_state0 && s > 0.0f)
         {
            _state0 = 1;
         }
         else if (_state0 && s < _hysteresis)
         {
            _state0 = 0;
            _state1 = 0;
            _state2 = 0;
         }

         if (!prev_state && _state0) // rising _state0
         {
            auto slope = s-_prev;
            prev_state = _state1;
            _state1 = discriminate(threshold1, slope, _slope_env1);
            if (!prev_state && _state1) // rising _state1
               _state2 = discriminate(threshold2, slope, _slope_env2);
         }

         _prev = s;
         return _state2;
      }

      bool operator()() const
      {
         return _state2;
      }

      void reset()
      {
         _prev = 0;
         _slope_env1 = _slope_env2 = 0;
         _state0 = _state1 = _state2 = 0;
      }

      float    _hysteresis;
      float    _prev = 0;
      float    _slope_env1 = 0;
      float    _slope_env2 = 0;
      bool     _state0:1;
      bool     _state1:1;
      bool     _state2:1;
   };

   ////////////////////////////////////////////////////////////////////////////
   // peak generates pulses that coincide with the peaks of a waveform. This
   // is accomplished by comparing the signal with the (slightly attenuated)
   // envelope of the signal (env) using a schmitt_trigger.
   //
   //    sensitivity: Envelope droop amount (attenuation) hysteresis:
   //    schmitt_trigger hysteresis amount
   //
   // The result is a bool corresponding to the peaks. Tip: For measuring
   // periods, look at the falling edges (i.e. the transitions from high to
   // low). The falling edges are more suitable and more accurate for marking
   // period edges.
   ////////////////////////////////////////////////////////////////////////////
   struct peak
   {
      peak(float sensitivity, float hysteresis)
       : _sensitivity(sensitivity), _cmp(hysteresis)
      {}

      peak(float sensitivity, decibel hysteresis)
       : _sensitivity(sensitivity), _cmp(as_float(hysteresis))
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

   ////////////////////////////////////////////////////////////////////////////
   // rising_edge detects rising edges (i.e returns 1 when the input
   // transitions from 0 to 1).
   ////////////////////////////////////////////////////////////////////////////
   struct rising_edge
   {
      constexpr bool operator()(bool val)
      {
         auto r = val && (_state != val);
         _state = val;
         return r;
      }
      bool _state = 0;
   };
}

#endif
