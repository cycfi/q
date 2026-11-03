/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SIN_COS_GEN_HPP_APRIL_26_2023)
#define CYCFI_Q_SIN_COS_GEN_HPP_APRIL_26_2023

#include <q/support/base.hpp>
#include <q/support/literals.hpp>
#include <cmath>
#include <utility>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // sin_cos_gen: Generates sine and cosine waves simultaneously. The
   // Chamberlin state variable filter is optimized for infinite Q
   // oscillation.
   //
   // 1. Suitable as a low frequency sine wave oscillator. The upper
   //    frequency limit for stability is roughly 1/6 sps.
   // 2. At low frequencies, the waveform symmetry is very pure, but it
   //    becomes skewed as you approach the upper limit.
   // 3. For low frequencies, we can reduce the calculation of the
   //    coefficient to 2*Pi*freq/sps.
   // 4. When utilized with fixed point or IEEE floating point, the
   //    oscallator should run forever without instability.
   //
   // T is the type of the coefficient and the state. sin_cos_gen is double.
   // Both are stable: the peak stays within 1e-6 of its start in float
   // (1e-8 in double) however long it runs, and at 0.1 Hz the frequency
   // errs by 7e-7 in float. A single-precision FPU does float in hardware.
   //
   // Source:
   // https://www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter/
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_sin_cos_gen
   {
      basic_sin_cos_gen(frequency f, float sps)
       : a(T(2) * fast_sin(T(pi) * as_float(f) / sps))
      {
      }

      std::pair<T, T> operator()()
      {
         s[0] = s[0] - a*s[1];
         s[1] = s[1] + a*s[0];
         return {s[1], s[0]};
      }

      void config(frequency f, float sps)
      {
         a = T(2) * fast_sin(T(pi) * as_float(f) / sps);
      }

      void reset(T sin = 0, T cos = 1)
      {
         s[0] = cos;
         s[1] = sin;
      }

      void midpoint()
      {
         reset(0, -1);
      }

      T a;
      T s[2] = {1, 0};
   };

   using sin_cos_gen = basic_sin_cos_gen<double>;
}

#endif
