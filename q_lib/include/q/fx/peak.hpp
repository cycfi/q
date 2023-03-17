/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PEAK_DECEMBER_7_2018)
#define CYCFI_Q_PEAK_DECEMBER_7_2018

#include <q/support/base.hpp>
#include <q/support/decibel.hpp>
#include <q/fx/schmitt_trigger.hpp>

namespace cycfi::q
{
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

      float const _sensitivity;
      schmitt_trigger _cmp;
   };
}

#endif
