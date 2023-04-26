/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_HANN_GEN_HPP_APRIL_26_2023)
#define CYCFI_Q_HANN_GEN_HPP_APRIL_26_2023

#include <q/synth/sin_cos_gen.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Hann window taper generator.
   //
   // The Hann window, also known as the Cosine Bell, is named after the
   // Austrian meteorologist Julius von Hann. The Hann window is a taper
   // formed with a cosine raised above zero. It is used as one of many
   // windowing functions for smoothing values. The The Hann window taper
   // belongs to both the cosine-sum and sine-power families. Unlike the
   // Hamming window, the Hann window's end points touch zero.
   //
   // The formula for the Hann window is:
   //    w(n) = 0.5*(1 - cos(2pin/(N-1)))
   //
   ////////////////////////////////////////////////////////////////////////////
   struct hann_gen
   {
      hann_gen(duration width, float sps)
       : cos{frequency(1.0f / as_float(width)), sps}
      {
      }

      float operator()()
      {
         return 0.5f * (1.0f - cos().second);
      }

      void config(duration width, float sps)
      {
         cos.config(frequency(1.0f / as_float(width)), sps);
      }

      sin_cos_gen cos;
   };
}

#endif
