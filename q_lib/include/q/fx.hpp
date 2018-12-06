/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FX_HPP_DECEMBER_24_2015)
#define CYCFI_Q_FX_HPP_DECEMBER_24_2015

#include <q/fx/map.hpp>
#include <q/fx/fast_downsample.hpp>
#include <q/fx/leaky_integrator.hpp>
#include <q/fx/moving_average.hpp>

#include <cmath>
#include <algorithm>
#include <q/literals.hpp>
#include <q/support.hpp>
#include <q/biquad.hpp>
#include <infra/assert.hpp>

namespace cycfi { namespace q
{
	using namespace literals;

   ////////////////////////////////////////////////////////////////////////////
   // Basic one pole low-pass filter (6dB/Oct)
   //
   //    y: current value
   //    a: coefficient
   //
   ////////////////////////////////////////////////////////////////////////////
   struct one_pole_lowpass
   {
      one_pole_lowpass(float a)
       : a(a)
      {}

      one_pole_lowpass(frequency f, std::uint32_t sps)
       : a(1.0 - fast_exp3(-2_pi * double(f) / sps))
      {}

      float operator()(float s)
      {
         return y += a * (s - y);
      }

      float operator()() const
      {
         return y;
      }

      one_pole_lowpass& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      void cutoff(frequency f, std::uint32_t sps)
      {
         a = 1.0 - fast_exp3(-2_pi * double(f) / sps);
      }

      float y = 0.0f, a;
   };

   ////////////////////////////////////////////////////////////////////////////
   // 3-point median filter
   ////////////////////////////////////////////////////////////////////////////
   struct median3
   {
      median3(float median_ = 0.0f)
       : _median(median_)
      {}

      float operator()(float a)
      {
         _median = std::max(std::min(a, b), std::min(std::max(a, b), c));
         c = b;
         b = a;
         return _median;
      }

      float operator()() const
      {
         return _median;
      }

      median3& operator=(float median_)
      {
         _median = median_;
         return *this;
      }

      float _median = 0.0f;
      float b = 0.0f;
      float c = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Basic one-pole allpass filter
   //
   //    a: location of the pole in the range -1..1
   //    y: current value
   //
   ////////////////////////////////////////////////////////////////////////////
   struct one_pole_allpass
   {
      one_pole_allpass(float a)
       : a(a)
      {}

      float operator()(float s)
      {
         auto out = y - a * s;
         y = s + a * out;
         return out;
      }

      float a, y = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // 2-Pole polyphase IIR allpass filter
   //
   //    a: coefficient
   //
   // See http://yehar.com/blog/?p=368
   ////////////////////////////////////////////////////////////////////////////
   struct polyphase_allpass
   {
      polyphase_allpass(float a)
       : a(a)
       , x1(0), x2(0), y1(0), y2(0)
      {}

      float operator()(float s)
      {
         auto r = a * (s + y2) - x2;

         // shift x1 to x2, s to x1
         x2 = x1;
         x1 = s;

         // shift y1 to y2, r to y1
         y2 = y1;
         y1 = r;

         return r;
      }

      float a;
      float x1, x2, y1, y2;
   };

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
   // The ratio parameter specifies the amount of gain applied. With ratio
   // within 0.0...1.0, the signal rising above the threshold is attenuated,
   // compressing the signal (compressor), while a ratio > 1.0, the signal
   // falling below the threshold is amplified, expanding the signal
   // (expander). Typically, you add some makeup gain after compression to
   // compensate for the gain reduction or increase.
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

      decibel  _threshold;
      float    _slope;
   };

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

      decibel  _threshold, _width, _lower, _upper;
      float    _slope;
   };

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

      decibel  _threshold;
      float    _slope;
   };

   ////////////////////////////////////////////////////////////////////////////
   // hard_limiter limits the signal above a specified threshold. s is the
   // input signal and env is the envelope of the signal obtained (e.g) using
   // the envelope_follower above.
   ////////////////////////////////////////////////////////////////////////////
   struct hard_limiter
   {
      hard_limiter(float threshold)
       : _threshold(threshold)
      {}

      float operator()(float s, float env)
      {
         if (env > _threshold)
            return s * fast_inverse(env);
         return s;
      }

      float _threshold;
   };

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
   // clip a signal to range -_max...+_max
   //
   //    _max: maximum value
   //
   ////////////////////////////////////////////////////////////////////////////
   struct clip
   {
      constexpr clip(float max = 1.0f)
       : _max(max)
      {}

      constexpr float operator()(float s) const
      {
         return (s > _max) ? _max : (s < -_max) ? -_max : s;
      }

      float _max;
   };

   ////////////////////////////////////////////////////////////////////////////
   // soft_clip a signal to range -1.0 to 1.0.
   ////////////////////////////////////////////////////////////////////////////
   struct soft_clip : clip
   {
      constexpr float operator()(float s) const
      {
         s = clip::operator()(s);
         return 1.5 * s - 0.5 * s * s * s;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // The differentiator returns the time derivative of the input (s).
   //
   //    x: delayed input sample
   //
   ////////////////////////////////////////////////////////////////////////////
   struct differentiator
   {
      differentiator()
       : x(0.0f) {}

      float operator()(float s)
      {
         auto val = s - x;
         x = s;
         return val;
      }

      float x;
   };

   ////////////////////////////////////////////////////////////////////////////
   // The integrator accumulates the input samples (s).
   //
   //    y:       current output value
   //    _gain:   gain
   //
   ////////////////////////////////////////////////////////////////////////////
   struct integrator
   {
      integrator(float gain = 0.1)
       : _gain(gain) {}

      float operator()(float s)
      {
         return (y += _gain * s);
      }

      integrator& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      float y = 0.0f, _gain;
   };

   ////////////////////////////////////////////////////////////////////////////
   // window_comparator. If input (s) exceeds a high threshold (_high), the
   // current state (y) becomes 1. Else, if input (s) is below a low
   // threshold (_low), the current state (y) becomes 0. If the state (s)
   // is in between the low and high thresholds, the previous state is kept.
   //
   //    _low:    low threshold
   //    _high:   high threshold
   //    y:       current state
   //
   ////////////////////////////////////////////////////////////////////////////
   struct window_comparator
   {
      window_comparator(float low = -0.5f, float high = 0.5f)
       : _low(low), _high(high)
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
   // DC blocker based on Julius O. Smith's document
   //
   //    y:       current value
   //    x:       delayed input sample
   //    _pole:   pole
   //
   // A smaller _pole value allows faster tracking of "wandering dc levels",
   // but at the cost of greater low-frequency attenuation.
   ////////////////////////////////////////////////////////////////////////////
   struct dc_block
   {
      dc_block(frequency f, std::uint32_t sps)
       : _pole(1.0f - (2_pi * double(f) / sps))
      {}

      float operator()(float s)
      {
         y = s - x + _pole * y;
         x = s;
         return y;
      }

      dc_block& operator=(bool y_)
      {
         y = y_;
         return *this;
      }

      void cutoff(frequency f, std::uint32_t sps)
      {
         _pole = 1.0f - (2_pi * double(f) / sps);
      }

      float _pole;
      float x = 0.0f;
      float y = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Basic one unit delay
   ////////////////////////////////////////////////////////////////////////////
   struct delay1
   {
      float operator()(float s)
      {
         auto r = y;
         y = s;
         return r;
      }

      float operator()() const
      {
         return y;
      }

      float y = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Basic two unit delay
   ////////////////////////////////////////////////////////////////////////////
   struct delay2
   {
      float operator()(float s)
      {
         return _d2(_d1(s));
      }

      delay1 _d1, _d2;
   };

   ////////////////////////////////////////////////////////////////////////////
   // central_difference is a differentiator with this time-domain expression:
   //
   //    y(n) = (x(n) - x(n-2)) / 2
   //
   // Unlike first-difference differentaior (see differentiator),
   // central_difference has better immunity to high-frequency noise. See
   // https://www.dsprelated.com/showarticle/35.php
   ////////////////////////////////////////////////////////////////////////////
   struct central_difference
   {
      float operator()(float s)
      {
         return (s - _d(s)) / 2;
      }

      delay2 _d;
   };
}}

#endif
