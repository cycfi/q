/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DYNAMIC_DECEMBER_7_2018)
#define CYCFI_Q_DYNAMIC_DECEMBER_7_2018

#include <algorithm>
#include <q/support/decibel.hpp>

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
   // The output is the compressed gain, also in decibels. Simply multiply
   // the signal by the result converted to float (lin_float) or double
   // (lin_double). For example:
   //
   //    auto gain = lin_float(comp(env));
   //    auto left_out = left_signal * gain;
   //    auto right_out = right_signal * gain;
   //
   // where left_signal and right_signal are stereo input signals and env
   // is the computed envelope (e.g) using an envelope follower.
   //
   // The ratio parameter specifies the amount of gain applied. With the
   // typical "n:1" notation for compressors, the ratio parameter is 1/n,
   // thereby the ratio for compressors is normally from 0.0...1.0. (e.g. 4:
   // 1 compression is 1/4 or 0.25). Signal rising above the threshold is
   // attenuated, compressing the signal. For every n dB above the
   // threshold, the output rises by 1 dB. For example, with a ratio of 4:1
   // (0.25), 4dB above the threshold is attenuated by 3dB.
   //
   // Typically, you add some makeup gain after compression to compensate for
   // the gain reduction.
   //
   // T is the decibel's type (see basic_decibel), here and in the classes
   // below. compressor is double; basic_compressor<float> keeps a dynamics
   // path in float for a single-precision FPU.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_compressor
   {
      using decibel_type = basic_decibel<T>;

                     basic_compressor(decibel_type threshold, float ratio);

      decibel_type   operator()(decibel_type env) const;
      void           threshold(decibel_type val);
      void           ratio(float ratio);
      decibel_type   threshold() const;
      float          ratio() const;

      decibel_type   _threshold;
      float          _slope;
   };

   using compressor = basic_compressor<double>;

   ////////////////////////////////////////////////////////////////////////////
   // The soft_knee_compressor compressor variant provides a more gradual
   // "soft knee" gain transition around the threshold, given a knee width.
   // See compressor above.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_soft_knee_compressor
   {
      using decibel_type = basic_decibel<T>;

                     basic_soft_knee_compressor(
                        decibel_type threshold
                      , decibel_type width
                      , float ratio
                     );

      decibel_type   operator()(decibel_type env) const;
      void           threshold(decibel_type val);
      void           width(decibel_type val);
      void           ratio(float ratio);
      decibel_type   threshold() const;
      decibel_type   width() const;
      float          ratio() const;

      decibel_type   _threshold, _width, _lower, _upper;
      float          _slope;
   };

   using soft_knee_compressor = basic_soft_knee_compressor<double>;

   ////////////////////////////////////////////////////////////////////////////
   // The expander is the inverse of the compressor. The expander adjusts the
   // gain when the signal falls below the threshold, attenuating the signal.
   // With the typical "1:n" notation for expanders, the ratio parameter is
   // n, thereby the ratio for expanders is normally from 1.0...inf. (e.g.
   // 1 : 4 expansion is 4). A ratio of 1 means no expansion. A ratio of
   // 1 : inf is a hard gate where no signal passes below the threshold.
   //
   // For every dB below the threshold, the output falls by n dB, so the
   // signal is attenuated by (n - 1) dB. For example, with a ratio of 1 : 4
   // (4), 1dB below the threshold is attenuated by 3dB, and the output
   // falls 4dB below the threshold.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_expander
   {
      using decibel_type = basic_decibel<T>;

                     basic_expander(decibel_type threshold, float ratio);

      decibel_type   operator()(decibel_type env) const;
      void           threshold(decibel_type val);
      void           ratio(float ratio);
      decibel_type   threshold() const;
      float          ratio() const;

      decibel_type   _threshold;
      float          _slope;
   };

   using expander = basic_expander<double>;

   ////////////////////////////////////////////////////////////////////////////
   // The agc (automatic gain control) compares the envelope, env, to a
   // reference, ref, and increases or decreases the gain to maintain a
   // constant output level. A Maximum gain that can be applied when the
   // signal falls below the reference. The max constructor parameter
   // specified this "maximum" gain.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_agc
   {
      using decibel_type = basic_decibel<T>;

                     basic_agc(decibel_type max);

      decibel_type   operator()(decibel_type env, decibel_type ref) const;
      void           max(decibel_type max_);
      decibel_type   max() const;

      decibel_type   _max;
   };

   using agc = basic_agc<double>;

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   // compressor
   template <typename T>
   inline basic_compressor<T>::basic_compressor(
      decibel_type threshold, float ratio)
    : _threshold(threshold)
    , _slope(1.0f - ratio)
   {}

   template <typename T>
   inline basic_decibel<T> basic_compressor<T>::operator()(
      decibel_type env) const
   {
      if (env <= _threshold)
         return dB<T>(0);
      return _slope * (_threshold - env);
   }

   template <typename T>
   inline void basic_compressor<T>::threshold(decibel_type val)
   {
      _threshold = val;
   }

   template <typename T>
   inline void basic_compressor<T>::ratio(float ratio)
   {
      _slope = 1.0f - ratio;
   }

   template <typename T>
   inline basic_decibel<T> basic_compressor<T>::threshold() const
   {
      return _threshold;
   }

   template <typename T>
   inline float basic_compressor<T>::ratio() const
   {
      return 1.0f - _slope;
   }

   ////////////////////////////////////////////////////////////////////////////
   // soft_knee_compressor
   template <typename T>
   inline basic_soft_knee_compressor<T>::basic_soft_knee_compressor(
      decibel_type threshold, decibel_type width, float ratio)
    : _threshold(threshold)
    , _width(width)
    , _lower(threshold - (_width * T(0.5)))
    , _upper(threshold + (_width * T(0.5)))
    , _slope(1.0f - ratio)
   {}

   template <typename T>
   inline basic_decibel<T> basic_soft_knee_compressor<T>::operator()(
      decibel_type env) const
   {
      if (env <= _lower)
      {
         return dB<T>(0);
      }
      else if (env <= _upper)
      {
         auto soft_slope = _slope * ((env - _lower) / _width) * T(0.5);
         return soft_slope * (_lower - env);
      }
      else
      {
         return _slope * (_threshold - env);
      }
   }

   template <typename T>
   inline void basic_soft_knee_compressor<T>::threshold(decibel_type val)
   {
      _threshold = val;
      _lower = _threshold - (_width * T(0.5));
      _upper = _threshold + (_width * T(0.5));
   }

   template <typename T>
   inline void basic_soft_knee_compressor<T>::width(decibel_type val)
   {
      _width = val;
      _lower = _threshold - (_width * T(0.5));
      _upper = _threshold + (_width * T(0.5));
   }

   template <typename T>
   inline void basic_soft_knee_compressor<T>::ratio(float ratio)
   {
      _slope = 1.0f - ratio;
   }

   template <typename T>
   inline basic_decibel<T> basic_soft_knee_compressor<T>::threshold() const
   {
      return _threshold;
   }

   template <typename T>
   inline basic_decibel<T> basic_soft_knee_compressor<T>::width() const
   {
      return _width;
   }

   template <typename T>
   inline float basic_soft_knee_compressor<T>::ratio() const
   {
      return 1.0f - _slope;
   }

   ////////////////////////////////////////////////////////////////////////////
   // expander
   template <typename T>
   inline basic_expander<T>::basic_expander(decibel_type threshold, float ratio)
    : _threshold(threshold)
    , _slope(ratio - 1.0f)
   {}

   template <typename T>
   inline basic_decibel<T> basic_expander<T>::operator()(
      decibel_type env) const
   {
      if (env >= _threshold)
         return dB<T>(0);
      return _slope * (env - _threshold);
   }

   template <typename T>
   inline void basic_expander<T>::threshold(decibel_type val)
   {
      _threshold = val;
   }

   template <typename T>
   inline void basic_expander<T>::ratio(float ratio)
   {
      _slope = ratio - 1.0f;
   }

   template <typename T>
   inline basic_decibel<T> basic_expander<T>::threshold() const
   {
      return _threshold;
   }

   template <typename T>
   inline float basic_expander<T>::ratio() const
   {
      return _slope + 1.0f;
   }

   ////////////////////////////////////////////////////////////////////////////
   // agc
   template <typename T>
   inline basic_agc<T>::basic_agc(decibel_type max)
    : _max(max)
   {}

   template <typename T>
   inline basic_decibel<T> basic_agc<T>::operator()(
      decibel_type env, decibel_type ref) const
   {
      auto g = ref - env;
      return std::min<decibel_type>(g, _max);
   }

   template <typename T>
   inline void basic_agc<T>::max(decibel_type max_)
   {
      _max = max_;
   }

   template <typename T>
   inline basic_decibel<T> basic_agc<T>::max() const
   {
      return _max;
   }
}

#endif
