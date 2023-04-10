/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_AGC_DECEMBER_7_2018)
#define CYCFI_Q_AGC_DECEMBER_7_2018

#include <q/support/base.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // The agc (automatic gain control) compares the envelope, env, to a
   // reference, ref, and increases or decreases the gain to maintain a
   // constant output level. A Maximum gain that can be applied when the
   // signal falls below the reference. The max constructor parameter
   // specified this "maximum" gain.
   //
   // Note that these are envelope processors that work in the logarithmic
   // domain (decibels) and process envelopes instead of the actual signal.
   // The output is the processed envelope, also in decibels. Simply
   // multiply the signal by the result converted to float (or double). For
   // example:
   //
   //    auto gain = as_float(agc(env));
   //    auto left_out = left_signal * gain;
   //    auto right_out = right_signal * gain;
   //
   // where left_signal and right_signal are stereo input signals and
   // envelope is the computed envelope (e.g) using an envelope follower.
   ////////////////////////////////////////////////////////////////////////////
   struct agc
   {
                  agc(decibel max);

      decibel     operator()(decibel env, decibel ref) const;
      void        max(decibel max_);
      decibel     max() const;

      decibel     _max;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline agc::agc(decibel max)
    : _max(max)
   {}

   inline decibel agc::operator()(decibel env, decibel ref) const
   {
      auto g = ref - env;
      return std::min<decibel>(g, _max);
   }

   inline void agc::max(decibel max_)
   {
      _max = max_;
   }

   inline decibel agc::max() const
   {
      return _max;
   }
}

#endif
