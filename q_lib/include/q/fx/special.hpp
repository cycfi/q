/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SPECIAL_HPP_DECEMBER_24_2015)
#define CYCFI_Q_SPECIAL_HPP_DECEMBER_24_2015

// #include <cmath>
// #include <algorithm>
#include <q/literals.hpp>
#include <q/support.hpp>
#include <q/fx.hpp>

namespace cycfi { namespace q
{
	using namespace literals;

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
