/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_HILBERT_QUADRATURE_HPP_DECEMBER_24_2015)
#define CYCFI_Q_HILBERT_QUADRATURE_HPP_DECEMBER_24_2015

#include <utility>
#include <q/fx/delay.hpp>

namespace cycfi::q
{
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
}

#endif
