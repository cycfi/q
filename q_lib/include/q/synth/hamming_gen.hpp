/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_HAMMING_GEN_HPP_APRIL_26_2023)
#define CYCFI_Q_HAMMING_GEN_HPP_APRIL_26_2023

#include <q/synth/sin_cos_gen.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Hamming window taper generator.
   //
   // The Hamming window, named for R. W. Hamming, is a window function that
   // tapers a signal towards its edges with non-zero endpoints. The Hamming
   // window features a moderate rolloff rate and optimized to minimize the
   // nearest side lobe, allowing for a fair mix of spectral resolution and
   // noise reduction.
   //
   // The formula for the Hamming window is:
   //    w(n) = 0.54 - 0.46 * cos(2pi*n/(N-1))
   //
   ////////////////////////////////////////////////////////////////////////////
   struct hamming_gen
   {
      hamming_gen(duration width, float sps)
       : cos{frequency(1.0f / as_float(width)), sps}
      {
      }

      float operator()()
      {
         return 0.54f - 0.46f * cos().second;
      }

      void config(duration width, float sps)
      {
         cos.config(frequency(1.0f / as_float(width)), sps);
      }

      void reset()
      {
         cos.reset();
      }

      void midpoint()
      {
         cos.midpoint();
      }

   private:

      sin_cos_gen cos;
   };
}

#endif
