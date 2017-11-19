/*=============================================================================
   Copyright (c) 2014-2017 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FX_HPP_DECEMBER_24_2015)
#define CYCFI_Q_FX_HPP_DECEMBER_24_2015

#include <cstdint>
#include <cmath>
#include <algorithm>
#include <q/support.hpp>

namespace cycfi { namespace q
{
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
   // k will also be the filter gain, so the final result should be
   // divided by k.
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

      T y = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Leaky Integrator
   ////////////////////////////////////////////////////////////////////////////
   struct leaky_integrator
   {
      leaky_integrator(float a = 0.995)
       : y(0)
       , a(a)
      {}

      leaky_integrator(float cutoff, uint32_t sps)
       : y(0.0f)
       , a(1.0f -(_2pi * cutoff/sps))
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

      float y;
      float a;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Basic one pole low-pass filter (6dB/Oct)
   ////////////////////////////////////////////////////////////////////////////
   struct one_pole_lp
   {
      // y: current value
      // a: coefficient

      one_pole_lp(float a)
       : y(0.0f), a(a)
      {}

      one_pole_lp(float freq, uint32_t sps)
       : y(0.0f), a(1.0f - std::exp(-_2pi * freq/sps))
      {}

      float operator()(float s)
      {
         return y += a * (s - y);
      }

      float operator()() const
      {
         return y;
      }

      one_pole_lp& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      float y, a;
   };

   ////////////////////////////////////////////////////////////////////////////
   // The envelope follower will follow the envelope of a signal with gradual
   // release (given by the r parameter). The signal decays exponentially if
   // the signal is below the peak.
   ////////////////////////////////////////////////////////////////////////////
   struct envelope_follower
   {
      // y: current value
      // d: decay

      envelope_follower(float r = 0.999f)
       : y(0.0f), r(r)
      {}

      envelope_follower(float release_time, uint32_t sps)
       : y(0.0f), r(std::exp(-1.0f / (sps * release_time)))
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

      envelope_follower& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      float y, r;
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
   // The result is a bool.
   ////////////////////////////////////////////////////////////////////////////
   struct schmitt_trigger
   {
      // y: current value
      // h: hysteresis

      schmitt_trigger(float h)
       : y(0.0f), h(h)
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

      float y;
      float h;
   };

   ////////////////////////////////////////////////////////////////////////////
   // clip a signal to range -m...+m
   ////////////////////////////////////////////////////////////////////////////
   struct clip
   {
      // m: maximum value

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
   // The differentiator returns the time derivative of the input (s).
   ////////////////////////////////////////////////////////////////////////////
   struct differentiator
   {
      // x: delayed input sample

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
   ////////////////////////////////////////////////////////////////////////////
   struct integrator
   {
      // y: current output value
      // g: gain

      integrator(float g = 0.1)
       : y(0.0f), g(g) {}

      float operator()(float s)
      {
         return (y += g * s);
      }

      integrator& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      float y, g;
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
   struct downsample
   {
      downsample()
       : x(0.0f)
      {}

      T operator()(T s1, T s2)
      {
         auto out = x + (s1 >> 1);
         x = s2 >> 2;
         return out + x;
      }

      T x;
   };

   ////////////////////////////////////////////////////////////////////////////
   // window_comparator. If input (s) exceeds a high threshold (h), the
   // current state (y) becomes 1. Else, if input (s) is below a low
   // threshold (l), the current state (y) becomes 0. If the state (s)
   // is in between the low and high thresholds, the previous state is kept.
   ////////////////////////////////////////////////////////////////////////////
   struct window_comparator
   {
      // l: low threshold
      // h: high threshold
      // y: current state

      window_comparator(float l = -0.5f, float h = 0.5f)
       : l(l), h(h), y(1.0f)
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
      bool y;
   };

   ////////////////////////////////////////////////////////////////////////////
   // DC blocker based on Julius O. Smith's document
   ////////////////////////////////////////////////////////////////////////////
   struct dc_block
   {
      // y: current value
      // x: delayed input sample
      // r: pole

      // A smaller r value allows faster tracking of "wandering dc levels",
      // but at the cost of greater low-frequency attenuation.

      dc_block(float r = 0.995)
       : r(r), x(0.0f), y(0.0f)
      {}

      dc_block(float cutoff, uint32_t sps)
       : r(1.0f - (_2pi * cutoff/sps)), x(0.0f), y(0.0f)
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

      float r, x, y;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Dynamic Smoothing Using Self Modulating Filter.
   // https://cytomic.com/files/dsp/DynamicSmoothing.pdf
   // Andrew Simper, Cytomic, 2014, andy@cytomic.com
   ////////////////////////////////////////////////////////////////////////////
   struct dynamic_smoothing
   {
      dynamic_smoothing(
         float freq
       , uint32_t sps
       , float sensitivity = 0.5f
      )
       : g0(compute_g0(freq, sps))
       , sense(sensitivity * 4)     // efficient linear cutoff mapping
       , lowl(0.0f)
       , low2(0.0f)
      {}

      float operator()(float s)
      {
         float bandz = lowl - low2;
         float g = std::min(g0 + sense * std::abs(bandz), 1.0f);

         lowl += g * (s - lowl);
         low2 = low2 + g * (lowl - low2);
         return low2;
      }

      static constexpr float compute_g0(float freq, uint32_t sps)
      {
         auto wc = freq / sps;
         auto gc = std::tan(pi * wc);
         return 2.0f * gc / (1.0f + gc);
      }

      float g0, sense, lowl, low2;
   };
}}

#endif
