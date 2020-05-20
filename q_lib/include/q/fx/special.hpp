/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SPECIAL_HPP_DECEMBER_24_2015)
#define CYCFI_Q_SPECIAL_HPP_DECEMBER_24_2015

#include <q/support/base.hpp>
#include <q/support/literals.hpp>
#include <q/fx/allpass.hpp>
#include <q/fx/delay.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Map the input, s (with expected value 0 to 1) to y1 to y2 using linear
   // interpolation. Example: if the range (y1, y2) is (0.5, 0.8), the input
   // value 0 is mapped to 0.5 while the value 1 is mapped to 0.8.
   //
   // Note that y1 can be greater than y2. Example: if the range (y1, y2) is
   // (1.0, 0.0), the input value 0 is mapped to 1 while the value 1 is
   // mapped to 0.
   ////////////////////////////////////////////////////////////////////////////
   struct map
   {
      constexpr map(float y1, float y2)
       : _y1(y1)
       , _y2(y2)
      {}

      constexpr float operator()(float s) const
      {
         return linear_interpolate(_y1, _y2, s);
      }

      void range(float y1, float y2)
      {
         _y1 = y1;
         _y2 = y2;
      }

      float _y1, _y2;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Fast Downsampling with antialiasing. A quick and simple method of
   // downsampling a signal by a factor of two with a useful amount of
   // antialiasing. Each source sample is convolved with { 0.25, 0.5, 0.25 }
   // before downsampling. (from http://www.musicdsp.org/)
   //
   // This class is templated on the native integer or floating point
   // sample type (e.g. uint16_t).
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct fast_downsample
   {
      constexpr T operator()(T s1, T s2)
      {
         auto out = x + (s1/2);
         x = s2/4;
         return out + x;
      }

      T x = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // DC blocker based on Julius O. Smith's document
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

      float _pole;      // pole
      float x = 0.0f;   // delayed input sample
      float y = 0.0f;   // current value
   };

   ////////////////////////////////////////////////////////////////////////////
   // The differentiator returns the time derivative of the input (s).
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

      float x; // delayed input sample
   };

   ////////////////////////////////////////////////////////////////////////////
   // central_difference is a differentiator with this time-domain expression:
   //
   //    y(n) = (x(n) - x(n-2)) / 2
   //
   // Unlike first-difference differentiator (see differentiator),
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

   ////////////////////////////////////////////////////////////////////////////
   // The integrator accumulates the input samples (s).
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

      float y = 0.0f;   // current output value
      float _gain;      // gain
   };

   ////////////////////////////////////////////////////////////////////////////
   // crossfade smoothly fades-in and fades-out two signals, `a` and `b`,
   // when a control argument, `ctrl`, falls below a given `pivot`. If `ctrl`
   // is above the pivot (e.g. -10dB) the gain of `a` is 1.0 and the gain of
   // `b` is 0.0. if `ctrl` falls below the pivot (e.g. -10dB), `a` fades-out
   // while `b` fades-in smoothly by (ctrl - pivot) decibels.
   //
   // For example, if `pivot` is -10dB, and `ctrl` is -13dB, the gain of `a`
   // is 0.708 (-3dB == -10dB - -13dB) and the gain of `b` is 0.3 (1.0 -
   // 0.708).
   ////////////////////////////////////////////////////////////////////////////
   struct crossfade
   {
      constexpr crossfade(decibel pivot)
       : _pivot(pivot)
      {}

      float operator()(float a, float b, decibel ctrl)
      {
         if (ctrl < _pivot)
         {
            auto xfade = float(ctrl - _pivot);
            return xfade * a + (1.0 - xfade) * b;
         }
         return a;
      }

      void pivot(decibel pivot_)
      {
         _pivot = pivot_;
      }

      decibel  _pivot;
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

   ////////////////////////////////////////////////////////////////////////////
   // hilbert_quadrature uses two all-pass IIR filters with a phase
   // difference of approximately 90 degrees over a range of frequencies
   // symmetric around Nyquist/2.
   //
   // This is probably the most efficient structure for implementing a
   // Hilbert transform (See http://yehar.com/blog/?p=368) by Olli Niemitalo.
   ////////////////////////////////////////////////////////////////////////////
   struct hilbert_quadrature
   {
      std::pair<float, float> operator()(float s)
      {
         return {
            _dly(_d(_c(_b(_a(s)))))
          , _z(_y(_x(_w(s))))
         };
      }

      polyphase_allpass _a{ 0.47940086558884 };
      polyphase_allpass _b{ 0.87621849353931 };
      polyphase_allpass _c{ 0.976597589508199 };
      polyphase_allpass _d{ 0.997499255935549 };

      polyphase_allpass _w{ 0.161758498367701 };
      polyphase_allpass _x{ 0.733028932341491 };
      polyphase_allpass _y{ 0.945349700329113 };
      polyphase_allpass _z{ 0.990599156684529 };

      delay1            _dly;
   };

   ////////////////////////////////////////////////////////////////////////////
   // monostable is a one shot pulse generator. A single pulse input
   // generates a timed pulse of given duration.
   ////////////////////////////////////////////////////////////////////////////
   struct monostable
   {
      monostable(duration d, std::uint32_t sps)
       : _n_samples(float(d) * sps)
      {}

      bool operator()(bool val)
      {
         if (_ticks == 0)
         {
            if (val)
               _ticks = _n_samples;
         }
         else
         {
            --_ticks;
         }
         return _ticks != 0;
      }

      std::uint32_t  _n_samples;
      std::uint32_t  _ticks = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // rising_edge detects rising edges (i.e returns 1 when the input
   // transitions from 0 to 1).
   ////////////////////////////////////////////////////////////////////////////
   struct rising_edge
   {
      constexpr bool operator()(bool val)
      {
         auto r = val && (_state != val);
         _state = val;
         return r;
      }
      bool _state = 0;
   };
}

#endif
