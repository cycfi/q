/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SPECIAL_HPP_DECEMBER_24_2015)
#define CYCFI_Q_SPECIAL_HPP_DECEMBER_24_2015

#include <q/literals.hpp>
#include <q/support.hpp>
#include <q/fx/all_pass.hpp>
#include <q/fx/delay.hpp>

namespace cycfi { namespace q
{
	using namespace literals;

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
}}

#endif
