/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FX_HPP_DECEMBER_24_2015)
#define CYCFI_Q_FX_HPP_DECEMBER_24_2015

#include <cmath>
#include <algorithm>
#include <q/literals.hpp>
#include <q/support.hpp>
#include <q/biquad.hpp>

namespace cycfi { namespace q
{
	using namespace literals;

   ////////////////////////////////////////////////////////////////////////////
   // var_fx: stores a single value
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct var_fx
   {
      T operator()(T s)
      {
         return y = s;
      }

      T operator()() const
      {
         return y;
      }

      T y;
   };

   template <typename T>
   inline var_fx<T> var(T val)
   {
      return { val };
   }

   ////////////////////////////////////////////////////////////////////////////
   // fixed_pt_leaky_integrator: If you want a fast filter for integers, use
   // a fixed point leaky-integrator. k will determine the effect of the
   // filter. Choose k to be a power of 2 for efficiency (the compiler will
   // optimize the computation using shifts). k = 16 is a good starting
   // point.
   //
   // This simulates the RC filter in digital form. The equation is:
   //
   //    y[i] = rho * y[i-1] + s
   //
   // where rho < 1. To avoid floating point, we use k instead, which
   // allows for integer operations. In terms of k, rho = 1 - (1 / k).
   // So the actual formula is:
   //
   //    y[i] += s - (y[i-1] / k);
   //
   // k will also be the filter gain, so the final result should be divided
   // by k. If you need to initialize the filter (y member) to a certain
   // state, you will also need to multiply the initial value by k.
   //
   ////////////////////////////////////////////////////////////////////////////
   template <int k, typename T = int>
   struct fixed_pt_leaky_integrator
   {
      typedef T result_type;

      T operator()(T s)
      {
         y += s - (y / k);
         return y;
      }

      T operator()() const
      {
         return y;
      }

      fixed_pt_leaky_integrator& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      T y = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Leaky Integrator
   ////////////////////////////////////////////////////////////////////////////
   struct leaky_integrator
   {
      leaky_integrator(float a = 0.995)
       : a(a)
      {}

      leaky_integrator(frequency f, std::uint32_t sps)
       : a(1.0f -(2_pi * double(f) / sps))
      {}

      float operator()(float s)
      {
         return y = s + a * (y - s);
      }

      float operator()() const
      {
         return y;
      }

      leaky_integrator& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      void cutoff(frequency f, std::uint32_t sps)
      {
         a = 1.0f -(2_pi * double(f) / sps);
      }

      float y = 0.0f, a;
   };

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
   // Exponential moving average approximates an arithmetic moving average by
   // multiplying the last result by some factor, and adding it to the next
   // sample multiplied by some other factor.
   //
   // If b = 2/(n+1), where n is the number of samples you would have used in
   // an arithmetic average, the exponential moving average will approximate
   // the arithmetic average pretty well.
   //
   //    n: the number of samples.
   //    y: current value
   //
   // See: https://www.dsprelated.com/showthread/comp.dsp/47981-1.php
   ////////////////////////////////////////////////////////////////////////////
   template <int n>
   struct exp_moving_average
   {
      static constexpr float b = 2.0f / (n + 1);
      static constexpr float b_ = 1.0f - b;

      exp_moving_average(float y_ = 0.0f)
       : y(y_)
      {}

      float operator()(float s)
      {
         return y = b * s + b_ * y;
      }

      float operator()() const
      {
         return y;
      }

      exp_moving_average& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      float y = 0.0f;
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
   // The envelope follower will follow the envelope of a signal with gradual
   // release (given by the release parameter). The signal decays
   // exponentially if the signal is below the peak.
   //
   //    y:          current value
   //    _attack:    attack
   //    _release:   release
   //
   ////////////////////////////////////////////////////////////////////////////
   struct envelope_follower
   {
      envelope_follower(duration attack, duration release, std::uint32_t sps)
       : _attack(fast_exp3(-2.0f / (sps * double(attack))))
       , _release(fast_exp3(-2.0f / (sps * double(release))))
      {}

      float operator()(float s)
      {
         s = std::abs(s);
         return y = s + ((s > y)? _attack : _release) * (y - s);
      }

      float operator()() const
      {
         return y;
      }

      envelope_follower& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      void config(duration attack, duration release, std::uint32_t sps)
      {
         _attack = fast_exp3(-2.0f / (sps * double(attack)));
         _release = fast_exp3(-2.0f / (sps * double(release)));
      }

      void attack(float attack_, std::uint32_t sps)
      {
         _attack = fast_exp3(-2.0f / (sps * attack_));
      }

      void release(float release_, std::uint32_t sps)
      {
         _release = fast_exp3(-2.0f / (sps * release_));
      }

      float y = 0.0f, _attack, _release;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Same as envelope follower above, but with attack = 0;
   //
   //    y:          current value
   //    _release:   release
   //
   ////////////////////////////////////////////////////////////////////////////
   struct peak_envelope_follower
   {
      peak_envelope_follower(duration release, std::uint32_t sps)
       : _release(fast_exp3(-2.0f / (sps * double(release))))
      {}

      float operator()(float s)
      {
         if (s > y)
            y = s;
         else
            y = s + _release * (y - s);
         return y;
      }

      float operator()() const
      {
         return y;
      }

      peak_envelope_follower& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      void release(float release_, std::uint32_t sps)
      {
         _release = fast_exp3(-2.0f / (sps * release_));
      }

      float y = 0.0f, _release;
   };

   ////////////////////////////////////////////////////////////////////////////
   // compressor_expander dynamically modulates the gain when the signal
   // rises above a specified threshold. Signal level tracking is done using
   // a separate envelope follower to make it possible to use different types
   // of envelope tracking schemes, the output of which is the supplied 'env'
   // argument to the function call operator (float s, float env). s is the
   // input signal and env is the envelope of the signal obtained (e.g) using
   // the envelope_follower above.
   //
   // The slope parameter specifies the amount of gain applied. With slope
   // within 0.0...1.0, the signal rising above the threshold is attenuated,
   // compressing the signal (compressor), while a slope > 1.0, the signal
   // rising above the threshold is amplified, expanding the signal
   // (expander). Typically, you add some makeup gain after compression or
   // expansion to compensate for the gain reduction or increase.
   ////////////////////////////////////////////////////////////////////////////
   struct compressor_expander
   {
      constexpr compressor_expander(float threshold, float slope)
       : _threshold(threshold)
       , _slope(slope)
      {}

      float operator()(float s, float env)
      {
         float gain = 1.0f;
         if (env > _threshold)
            gain -= (env - _threshold) * _slope;
         return s * gain;
      }

      float _threshold;
      float _slope;
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
