/*=============================================================================
   Copyright (c) 2014-2018 Cycfi Research. All rights reserved.

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
   // fixed_pt_leaky_integrator: If you a fast filter for integers, use
   // a fixed point leaky-integrator. k will determine the effect of the
   // filter. Choose k to be a power of 2 for efficiency (the compiler
   // will optimize the computation using shifts). k = 16 is a good starting
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
       : a(1.0 - std::exp(-2_pi * double(f) / sps))
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
         a = 1.0 - std::exp(-2_pi * double(f) / sps);
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
   // release (given by the r parameter). The signal decays exponentially if
   // the signal is below the peak.
   //
   //    y: current value
   //    a: attack
   //    r: release
   //
   ////////////////////////////////////////////////////////////////////////////
   struct envelope_follower
   {
      envelope_follower(duration attack, duration release, std::uint32_t sps)
       : a(std::exp(-2.0f / (sps * double(attack))))
       , r(std::exp(-2.0f / (sps * double(release))))
      {}

      float operator()(float s)
      {
         s = std::abs(s);
         return y = s + ((s > y)? a : r) * (y - s);
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
         a = std::exp(-2.0f / (sps * double(attack)));
         r = std::exp(-2.0f / (sps * double(release)));
      }

      void attack(float attack_, std::uint32_t sps)
      {
         a = std::exp(-2.0f / (sps * attack_));
      }

      void release(float release_, std::uint32_t sps)
      {
         r = std::exp(-2.0f / (sps * release_));
      }

      float y = 0.0f, a, r;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Same as envelope follower above, but with attack = 0;
   //
   //    y: current value
   //    r: release
   //
   ////////////////////////////////////////////////////////////////////////////
   struct peak_envelope_follower
   {
      peak_envelope_follower(duration release, std::uint32_t sps)
       : r(std::exp(-2.0f / (sps * double(release))))
      {}

      float operator()(float s)
      {
         if (s > y)
            y = s;
         else
            y = s + r * (y - s);
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
         r = std::exp(-2.0f / (sps * release_));
      }

      float y = 0.0f, a, r;
   };

   ////////////////////////////////////////////////////////////////////////////
   // The hard_limiter limits the signal to 1.0f. s is the input signal
   // and env is the envelope of the signal obtained (e.g) using the
   // envelope_follower above.
   ////////////////////////////////////////////////////////////////////////////
   struct hard_limiter
   {
      float operator()(float s, float env)
      {
         if (env > 1.0f)
            return s * fast_inverse(env);
         return s;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // The output of a simple comparator is determined by its inputs. The output
   // is high (1) if the positive input (spos) is greater than the negative
   // input (sneg). Otherwise, the output is low (0).
   //
   // The schmitt trigger adds some hysteresis (h) to improve noise immunity
   // and minimize multiple triggering by adding and subtracting a certain
   // fraction of the previous output (y) back to the positive input (spos).
   // hysteresis is the fraction (should be less than < 1.0) that determines
   // how much is added or subtracted. By doing so, the comparator "bar" is
   // raised or lowered depending on the previous state.
   //
   //    y: current value
   //    h: hysteresis
   //
   // Note: the result is a bool.
   ////////////////////////////////////////////////////////////////////////////
   struct schmitt_trigger
   {
      schmitt_trigger(float h)
       : h(h)
      {}

      bool operator()(float spos, float sneg)
      {
         auto delta = (y - spos) * h;
         y = ((spos + delta) > sneg) ? 1.0f : -1.0f;
         return y > 0.0f;
      }

      bool operator()() const
      {
         return y > 0.0f;
      }

      schmitt_trigger& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      float y = 0.0f, h;
   };

   ////////////////////////////////////////////////////////////////////////////
   // clip a signal to range -m...+m
   //
   //    m: maximum value
   ////////////////////////////////////////////////////////////////////////////
   struct clip
   {
      constexpr clip(float m = 1.0f)
       : m(m)
      {}

      constexpr float operator()(float s) const
      {
         return (s > m) ? m : (s < -m) ? -m : s;
      }

      float m;
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
   //    y: current output value
   //    g: gain
   //
   ////////////////////////////////////////////////////////////////////////////
   struct integrator
   {
      integrator(float g = 0.1)
       : g(g) {}

      float operator()(float s)
      {
         return (y += g * s);
      }

      integrator& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      float y = 0.0f, g;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Fast Downsampling with antialiasing. A quick and simple method of
   // downsampling a signal by a factor of two with a useful amount of
   // antialiasing. Each source sample is convolved with { 0.25, 0.5, 0.25 }
   // before downsampling. (from http://www.musicdsp.org/)
   //
   // This class is templated on the native integer sample type
   // (e.g. uint16_t).
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct fast_downsample
   {
      T operator()(T s1, T s2)
      {
         auto out = x + (s1 >> 1);
         x = s2 >> 2;
         return out + x;
      }

      T x = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // window_comparator. If input (s) exceeds a high threshold (h), the
   // current state (y) becomes 1. Else, if input (s) is below a low
   // threshold (l), the current state (y) becomes 0. If the state (s)
   // is in between the low and high thresholds, the previous state is kept.
   //
   //    l: low threshold
   //    h: high threshold
   //    y: current state
   //
   ////////////////////////////////////////////////////////////////////////////
   struct window_comparator
   {
      window_comparator(float l = -0.5f, float h = 0.5f)
       : l(l), h(h)
      {}

      bool operator()(float s)
      {
         if (s < l)
            y = 0;
         else if (s > h)
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

      float l, h;
      bool y = 1;
   };

   ////////////////////////////////////////////////////////////////////////////
   // DC blocker based on Julius O. Smith's document
   //
   //    y: current value
   //    x: delayed input sample
   //    r: pole
   //
   // A smaller r value allows faster tracking of "wandering dc levels",
   // but at the cost of greater low-frequency attenuation.
   ////////////////////////////////////////////////////////////////////////////
   struct dc_block
   {
      dc_block(frequency f, std::uint32_t sps)
       : r(1.0f - (2_pi * double(f) / sps))
      {}

      float operator()(float s)
      {
         y = s - x + r * y;
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
         r = 1.0f - (2_pi * double(f) / sps);
      }

      float r;
      float x = 0.0f;
      float y = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // dynamic_smoother based on Dynamic Smoothing Using Self Modulating Filter
   // by Andrew Simper, Cytomic, 2014, andy@cytomic.com
   //
   //    https://cytomic.com/files/dsp/DynamicSmoothing.pdf
   //
   // A robust and inexpensive dynamic smoothing algorithm based on using the
   // bandpass output of a 2 pole multimode filter to modulate its own cutoff
   // frequency. The bandpass signal is a meaure of how much the signal is
   // "changing" so is useful to increase the cutoff frequency dynamically
   // and allow for faster tracking when the input signal is changing more.
   // The absolute value of the bandpass signal is used since either a change
   // upwards or downwards should increase the cutoff.
   //
   ////////////////////////////////////////////////////////////////////////////
   struct dynamic_smoother
   {
      dynamic_smoother(frequency base, std::uint32_t sps)
       : dynamic_smoother(base, 0.5, sps)
      {}

      dynamic_smoother(frequency base, float sensitivity, std::uint32_t sps)
       : sense(sensitivity * 4.0f)  // efficient linear cutoff mapping
       , wc(double(base) / sps)
      {
         auto gc = std::tan(pi * wc);
         g0 = 2.0f * gc / (1.0f + gc);
      }

      float operator()(float s)
      {
         auto lowlz = low1;
         auto low2z = low2;
         auto bandz = lowlz - low2z;
         auto g = std::min(g0 + sense * std::abs(bandz), 1.0f);
         low1 = lowlz + g * (s - lowlz);
         low2 = low2z + g * (low1 - low2z);
         return low2z;
      }

      void base_frequency(frequency base, std::uint32_t sps)
      {
         wc = double(base) / sps;
         auto gc = std::tan(pi * wc);
         g0 = 2.0f * gc / (1.0f + gc);
      }

      float sense, wc, g0;
      float low1 = 0.0f;
      float low2 = 0.0f;
   };
}}

#endif
