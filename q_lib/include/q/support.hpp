/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SUPPORT_HPP_DECEMBER_20_2015)
#define CYCFI_Q_SUPPORT_HPP_DECEMBER_20_2015

#include <type_traits>
#include <cstdint>
#include <cstring>
#include <infra/support.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // Some macros
   ////////////////////////////////////////////////////////////////////////////
   #if defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)
   # define Q_SATURATES_FLOAT_TO_INT_CONVERSION(true)
   #else
   # define Q_SATURATES_FLOAT_TO_INT_CONVERSION(false)
   #endif

   ////////////////////////////////////////////////////////////////////////////
   // pi
   ////////////////////////////////////////////////////////////////////////////
   constexpr double pi = 3.1415926535897932384626433832795;

   ////////////////////////////////////////////////////////////////////////////
   // abs (we need ot here because std::abs may not be constexpr)
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   constexpr T abs(T x)
   {
      return (x < T(0))? -x : x;
   }

   ////////////////////////////////////////////////////////////////////////////
   // fast tan approximation (from http://www.musicdsp.org/)
   ////////////////////////////////////////////////////////////////////////////
   constexpr float fast_tan(float x)
   {
      float sqr = x * x;
      float r = 2.033e-01f;
      r *= sqr;
      r += 3.1755e-01f;
      r *= sqr;
      r += 1.0f;
      r *= x;
      return r;
   }

   ////////////////////////////////////////////////////////////////////////////
   // fast pade-approximation of the tanh function (x should be: -3 <= x <= 3)
   ////////////////////////////////////////////////////////////////////////////
   constexpr float fast_rational_tanh(float x)
   {
      return x * (27 + x*x) / (27 + 9*x*x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // fast exp approximation (from http://www.musicdsp.org/)
   ////////////////////////////////////////////////////////////////////////////
   constexpr float fast_exp(float x)
   {
      return
         (362880.f + x *
            (362880.f + x *
               (181440.f + x *
                  (60480.f + x *
                     (15120.f + x *
                        (3024.f + x *
                           (504.f + x *
                              (72.f + x *
                                 (9.f + x)
         )))))))) * 2.75573192e-6f;
   }

   ////////////////////////////////////////////////////////////////////////////
   // fast fast_pow2 approximation (from http://tinyurl.com/ybrosuvw)
   ////////////////////////////////////////////////////////////////////////////
   float fast_pow2(float x)
   {
      // store address of float as std::int32_t pointer
      auto* px = reinterpret_cast<std::int32_t*>(&x);

      // temporary value for truncation
      float tx = (x - 0.5f) + (3 << 22);

      // integer power of 2
      auto const lx = *reinterpret_cast<std::int32_t*>(&tx) - 0x4b400000;

      // float remainder of power of 2
      float const dx = x - float(lx);

      // cubic apporoximation of 2^x for x in the range [0, 1]
      x = 1.0f +  dx*(0.6960656421638072f +
                  dx*(0.224494337302845f +
                  dx*(0.07944023841053369f)));

      // add integer power of 2 to exponent
      *px += lx << 23;
      return x;
   }

   ////////////////////////////////////////////////////////////////////////////
   // linear interpolation: Interpolates a value linearly between y1 and y2
   // given mu. If mu is 0, the result is y1. If mu is 1, then the result is
   // y2.
   ////////////////////////////////////////////////////////////////////////////
   constexpr float linear_interpolate(float y1, float y2, float mu)
   {
      return y1 + mu * (y2 - y1);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast reciprocal. See http://tinyurl.com/lgmnsyg. We want to avoid
   // multiplication. So instead of 1.0f/val, we use this function which
   // is a inaccurate, but fast substitute. It works by negating the exponent
   // which is assumed to be IEEE754.
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_inverse(float val)
   {
      auto x = reinterpret_cast<std::int32_t&>(val);
      x = 0x7EF311C2 - x;
      return reinterpret_cast<float&>(x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast division using multiplication and fast_inverse
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_div(float a, float b)
   {
      return a * fast_inverse(b);
   }
}}

#endif
