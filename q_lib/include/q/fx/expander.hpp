/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXPANDER_DECEMBER_7_2018)
#define CYCFI_Q_EXPANDER_DECEMBER_7_2018

#include <q/support/base.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // expander dynamically modulate the gain when the signal envelope falls
   // below a specified threshold. Envelope tracking is done using an
   // external envelope follower to make it possible to use different types
   // of envelope tracking schemes, the output of which is the supplied 'env'
   // argument to the function call operator operator()(decibel env) where
   // env is the envelope of the signal in decibels obtained (e.g) using the
   // envelope_follower.
   //
   // Note that these are envelope processors that work in the logarithmic
   // domain (decibels) and process envelopes instead of the actual signal.
   // The output is the expanded envelope, also in decibels. Simply multiply
   // the signal by the result converted to float (or double). For example:
   //
   //    auto gain = as_float(exp(env));
   //    auto left_out = left_signal * gain;
   //    auto right_out = right_signal * gain;
   //
   // where left_signal and right_signal are stereo input signals and
   // envelope is the computed envelope (e.g) using an envelope follower.
   //
   // The expander is the inverse of the compressor. The expander adjusts the
   // gain when the signal falls below the threshold, attenuating the signal.
   // With the typical "1:n" notation for expanders, the ratio parameter is
   // n, thereby the ratio for expanders is normally from 0.0...inf. (e.g.
   // 1 : 4 expansion is 4). A ratio of 1 : inf is a hard gate where no
   // signal passes below the threshold.
   //
   // For every dB below the threshold, the signal is attenuated by n dB. For
   // example, with a ratio of 4 : 1 (4), 1dB below the threshold is
   // attenuated by 4dB.
   ////////////////////////////////////////////////////////////////////////////
   struct expander
   {
                  expander(decibel threshold, float ratio);

      decibel     operator()(decibel env) const;
      void        threshold(decibel val);
      void        ratio(float ratio);
      decibel     threshold() const;
      float       ratio() const;

      decibel     _threshold;
      float       _slope;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline expander::expander(decibel threshold, float ratio)
    : _threshold(threshold)
    , _slope(ratio)
   {}

   inline decibel expander::operator()(decibel env) const
   {
      if (env >= _threshold)
         return 0_dB;
      return _slope * (env - _threshold);
   }

   inline void expander::threshold(decibel val)
   {
      _threshold = val;
   }

   inline void expander::ratio(float ratio)
   {
      _slope = ratio;
   }

   inline decibel expander::threshold() const
   {
      return _threshold;
   }

   inline float expander::ratio() const
   {
      return _slope;
   }
}

#endif
