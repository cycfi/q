/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SUPPORT_BASE_HPP_DECEMBER_20_2015)
#define CYCFI_Q_SUPPORT_BASE_HPP_DECEMBER_20_2015

#include <type_traits>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <infra/support.hpp>
#include <q/detail/fast_math.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // min_max_range
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct min_max_range
   {
      T min, max;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Some macros
   ////////////////////////////////////////////////////////////////////////////

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
   // fast exp Taylor series approximations (from http://www.musicdsp.org/)
   ////////////////////////////////////////////////////////////////////////////
   constexpr float fast_exp3(float x)
   {
      return (6+x*(6+x*(3+x)))*0.16666666f;
   }

   constexpr float fast_exp4(float x)
   {
      return (24+x*(24+x*(12+x*(4+x))))*0.041666666f;
   }

   constexpr float fast_exp5(float x)
   {
      return (120+x*(120+x*(60+x*(20+x*(5+x)))))*0.0083333333f;
   }

   constexpr float fast_exp6(float x)
   {
      return (720+x*(720+x*(360+x*(120+x*(30+x*(6+x))))))*0.0013888888f;
   }

   constexpr float fast_exp7(float x)
   {
      return (5040+x*(5040+x*(2520+x*
         (840+x*(210+x*(42+x*(7+x)))))))*0.00019841269f;
   }

   constexpr float fast_exp8(float x)
   {
      return (40320+x*(40320+x*(20160+x*(6720+x*
         (1680+x*(336+x*(56+x*(8+x))))))))*2.4801587301e-5f;
   }

   constexpr float fast_exp9(float x)
   {
      return (362880+x*(362880+x*(181440+x*(60480+x*
         (15120+x*(3024+x*(504+x*(72+x*(9+x)))))))))*2.75573192e-6f;
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

   ////////////////////////////////////////////////////////////////////////////
   // Fast fast_log2
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_log2(float x)
   {
      return fasterlog2(x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast fast_pow2
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_pow2(float x)
   {
      return fasterpow2(x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast log10
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_log10(float x)
   {
      return 0.301029995663981f * fast_log2(x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast pow10
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_pow10(float x)
   {
      return fasterpow(10, x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast random number generator
   ////////////////////////////////////////////////////////////////////////////
   inline int fast_rand()
   {
      static unsigned seed = 87263876;
      seed = (214013 * seed + 2531011);
      return (seed >> 16) & 0x7FFF;
   }

   ////////////////////////////////////////////////////////////////////////////
   // abs_within
   ////////////////////////////////////////////////////////////////////////////
   inline bool abs_within(float a, float b, float eps)
   {
      return abs(a-b) <= eps;
   }

   ////////////////////////////////////////////////////////////////////////////
   // rel_within
   ////////////////////////////////////////////////////////////////////////////
   inline bool rel_within(float a, float b, float eps)
   {
      return abs(a-b) <= eps * std::max(abs(a), abs(b));
   }
}

#endif
