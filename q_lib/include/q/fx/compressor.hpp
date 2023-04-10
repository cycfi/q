/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_COMPRESSOR_DECEMBER_7_2018)
#define CYCFI_Q_COMPRESSOR_DECEMBER_7_2018

#include <q/support/base.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // compressor dynamically modulates the gain when the signal envelope
   // rises above a specified threshold. Envelope tracking is done using an
   // external envelope follower to make it possible to use different types
   // of envelope tracking schemes, the output of which is the supplied 'env'
   // argument to the function call operator operator()(decibel env) where
   // env is the envelope of the signal in decibels obtained (e.g) using the
   // envelope_follower.
   //
   // Note that these are envelope processors that work in the logarithmic
   // domain (decibels) and process envelopes instead of the actual signal.
   // The output is the compressed envelope, also in decibels. Simply
   // multiply the signal by the result converted to float (or double). For
   // example:
   //
   //    auto gain = as_float(comp(env));
   //    auto left_out = left_signal * gain;
   //    auto right_out = right_signal * gain;
   //
   // where left_signal and right_signal are stereo input signals and
   // envelope is the computed envelope (e.g) using an envelope follower.
   //
   // The ratio parameter specifies the amount of gain applied. With the
   // typical "n:1" notation for compressors, the ratio parameter is 1/n,
   // thereby the ratio for compressors is normally from 0.0...1.0. (e.g. 4:
   // 1 compression is 1/4 or 0.25). Signal rising above the threshold is
   // attenuated, compressing the signal. For every dB above the threshold,
   // the signal is attenuated by n dB. For example, with a ratio of 4:1
   // (0.25), 1dB above the threshold is attenuated by 4dB.
   //
   // Typically, you add some makeup gain after compression to compensate for
   // the gain reduction.
   ////////////////////////////////////////////////////////////////////////////
   struct compressor
   {
                  compressor(decibel threshold, float ratio);

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
   inline compressor::compressor(decibel threshold, float ratio)
    : _threshold(threshold)
    , _slope(1.0f - ratio)
   {}

   inline decibel compressor::operator()(decibel env) const
   {
      if (env <= _threshold)
         return 0_dB;
      return _slope * (_threshold - env);
   }

   inline void compressor::threshold(decibel val)
   {
      _threshold = val;
   }

   inline void compressor::ratio(float ratio)
   {
      _slope = 1.0f - ratio;
   }

   inline decibel compressor::threshold() const
   {
      return _threshold;
   }

   inline float compressor::ratio() const
   {
      return 1.0f - _slope;
   }
}

#endif
