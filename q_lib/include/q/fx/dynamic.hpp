/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DYNAMIC_DECEMBER_7_2018)
#define CYCFI_Q_DYNAMIC_DECEMBER_7_2018

#include <q/support/base.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // compressor (including variant soft_knee_compressor) and expander
   // dynamically modulate the gain when the signal envelope rises above
   // (compressor) or falls below (expander) a specified threshold. Envelope
   // tracking is done using a separate envelope follower to make it possible
   // to use different types of envelope tracking schemes, the output of
   // which is the supplied 'env' argument to the function call operator
   // operator ()(decibel env) where env is the envelope of the signal in
   // decibels obtained (e.g) using the envelope_follower above.
   //
   // The soft_knee_compressor variant provides a more gradual "soft knee"
   // gain transition around the threshold, given a knee width.
   //
   // Note that these are gain processors that work in the logarithmic domain
   // (decibels) and process gain instead of the actual signal. The output is
   // the compressed or expanded gain, also in decibels. Simply multiply the
   // signal by the result converted to float (or double). For example:
   //
   //    auto gain = float(comp(envelope));
   //    auto left_out = left_signal * gain;
   //    auto right_out = right_signal * gain;
   //
   // where left_signal and right_signal are stereo input signals and
   // envelope is the computed envelope (e.g) using an envelope follower.
   //
   // The ratio parameter specifies the amount of gain applied. With the
   // typical "n:1" notation for compressors, the ratio parameter is 1/n,
   // thereby the ratio for compressors is normally from 0.0...1.0. (e.g.
   // 4 : 1 compression is 1/4 or 0.25). Signal rising above the threshold is
   // attenuated, compressing the signal (compressor). For every dB above the
   // threshold, the signal is atenuated by n dB. For example, with a ratio
   // of 4 : 1 (0.25), 1dB above the threshold is attenuated by 4dB.
   //
   // Typically, you add some makeup gain after compression to compensate for
   // the gain reduction.
   ////////////////////////////////////////////////////////////////////////////
   struct compressor
   {
      constexpr compressor(decibel threshold, float ratio)
       : _threshold(threshold)
       , _slope(1.0f - ratio)
      {}

      decibel operator()(decibel env)
      {
         if (env <= _threshold)
            return 0_dB;
         return _slope * (_threshold - env);
      }

      void threshold(decibel val)
      {
         _threshold = val;
      }

      void ratio(float ratio)
      {
         _slope = 1.0f - ratio;
      }

      decibel  _threshold;
      float    _slope;
   };

   ////////////////////////////////////////////////////////////////////////////
   struct soft_knee_compressor
   {
      constexpr soft_knee_compressor(decibel threshold, decibel width, float ratio)
       : _threshold(threshold)
       , _width(width)
       , _lower(threshold - (_width * 0.5))
       , _upper(threshold + (_width * 0.5))
       , _slope(1.0f - ratio)
      {}

      decibel operator()(decibel env)
      {
         if (env <= _lower)
         {
            return 0_dB;
         }
         else if (env <= _upper)
         {
            auto soft_slope = _slope * ((env - _lower) / _width) * 0.5;
            return soft_slope * (_lower - env);
         }
         else
         {
            return _slope * (_threshold - env);
         }
      }

      void threshold(decibel val)
      {
         _threshold = val;
         _lower = _threshold - (_width * 0.5);
         _upper = _threshold + (_width * 0.5);
      }

      void width(decibel val)
      {
         _width = val;
         _lower = _threshold - (_width * 0.5);
         _upper = _threshold + (_width * 0.5);
      }

      void ratio(float ratio)
      {
         _slope = 1.0f - ratio;
      }

      decibel  _threshold, _width, _lower, _upper;
      float    _slope;
   };

   ////////////////////////////////////////////////////////////////////////////
   // The expander is the inverse of the compressor. The expander adjusts the
   // gain when the signal falls below the threshold, attenuating the signal.
   // With the typical "1:n" notation for expanders, the ratio parameter is
   // n, thereby the ratio for compressors is normally from 0.0...inf. (e.g.
   // 1 : 4 expansion is 4). A ratio of 1 : inf is a hard gate where no
   // signal passes below the threshold.
   //
   // For every dB below the threshold, the signal is atenuated by n dB. For
   // example, with a ratio of 4 : 1 (4), 1dB below the threshold is
   // attenuated by 4dB.
   ////////////////////////////////////////////////////////////////////////////
   struct expander
   {
      constexpr expander(decibel threshold, float ratio)
       : _threshold(threshold)
       , _slope(ratio)
      {}

      decibel operator()(decibel env)
      {
         if (env >= _threshold)
            return 0_dB;
         return _slope * (env - _threshold);
      }

      void threshold(decibel val)
      {
         _threshold = val;
      }

      void ratio(float ratio)
      {
         _slope = ratio;
      }

      decibel  _threshold;
      float    _slope;
   };

   ////////////////////////////////////////////////////////////////////////////
   // The agc (automatic gain control) compares the envelope, env, to a
   // reference, ref, and increases or decreases the gain to maintain a
   // constant output level.
   ////////////////////////////////////////////////////////////////////////////
   struct agc
   {
      constexpr agc(decibel max)
       : _max(max)
      {}

      decibel operator()(decibel env, decibel ref)
      {
         auto g = ref - env;
         if (g > _max)
            return _max - (g - _max);
         return g;
      }

      void max(decibel max_)
      {
         _max = max_;
      }

      decibel  _max;
   };
}

#endif
