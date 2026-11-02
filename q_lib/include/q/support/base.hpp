/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SUPPORT_BASE_HPP_DECEMBER_20_2015)
#define CYCFI_Q_SUPPORT_BASE_HPP_DECEMBER_20_2015

#include <type_traits>
#include <cstdint>
#include <span>
#include <cstring>
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <utility>
#include <infra/support.hpp>
#include <q/detail/fast_math.hpp>
#include <q/utility/interpolation_primitives.hpp>

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
   // byte_span: a view of bytes, the currency of anything that moves
   // whole messages or records around, MIDI included.
   ////////////////////////////////////////////////////////////////////////////
   using byte_span = std::span<std::uint8_t const>;

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
   // Fast trigonometric functions
   //
   // fast sine variants: x in [ -pi, pi ]
   // fast cosine variants: x in [ -pi, pi ]
   // fast tangent variants: x in [ -pi/2, pi/2 ]
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_tan(float x)
   {
      return fasttan(x);
   }

   inline float faster_tan(float x)
   {
      return fastertan(x);
   }

   inline float fast_sin(float x)
   {
      return fastsin(x);
   }

   inline float faster_sin(float x)
   {
      return fastersin(x);
   }

   inline float fast_cos(float x)
   {
      return fastcos(x);
   }

   inline float faster_cos(float x)
   {
      return fastercos(x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // principal_arg: an angle wrapped to [-pi, pi], for a phase that keeps
   // growing, such as a phase vocoder's. 2 pi is subtracted in two parts
   // (Cody and Waite): 6.28125 has so few significant bits that k times it
   // is exact, and the small remainder carries the rest.
   ////////////////////////////////////////////////////////////////////////////
   inline float principal_arg(float x)
   {
      constexpr float two_pi_hi = 6.28125f;
      constexpr float two_pi_lo = float(2 * pi - 6.28125);
      constexpr float inv_two_pi = float(1 / (2 * pi));

      // Rounding can leave r a hair past pi either way: the same angle,
      // held to the range (fmin and fmax need no compare).
      float k = std::floor(x * inv_two_pi + 0.5f);
      float r = (x - k * two_pi_hi) - k * two_pi_lo;
      return std::fmin(std::fmax(r, -float(pi)), float(pi));
   }

   ////////////////////////////////////////////////////////////////////////////
   // fast_sincos: the sine and cosine of any angle, as {sin, cos}, sharing
   // one evaluation. The angle is reduced to the nearest quarter turn (Cody
   // and Waite, as principal_arg), leaving at most pi/4, where Taylor
   // polynomials to x^9 and x^8 are good to about 2e-9; the quarter's two
   // low bits then swap and sign the pair, with no branch.
   ////////////////////////////////////////////////////////////////////////////
   inline std::pair<float, float> fast_sincos(float x)
   {
      constexpr float half_pi_hi = 1.5703125f;
      constexpr float half_pi_lo = float(pi / 2 - 1.5703125);
      constexpr float two_over_pi = float(2 / pi);

      float const q = std::floor(x * two_over_pi + 0.5f);
      float const r = (x - q * half_pi_hi) - q * half_pi_lo;
      float const r2 = r * r;
      float const s = r * (1 + r2 * (-1.0f/6 + r2 * (1.0f/120
         + r2 * (-1.0f/5040 + r2 * (1.0f/362880)))));
      float const c = 1 + r2 * (-0.5f + r2 * (1.0f/24 + r2 * (-1.0f/720
         + r2 * (1.0f/40320))));

      // Quarter n: sin, cos = s, c; c, -s; -s, -c; -c, s.
      auto const n = std::uint32_t(std::int32_t(q));
      auto const swap = std::uint32_t(0) - (n & 1);
      auto const sb = std::bit_cast<std::uint32_t>(s);
      auto const cb = std::bit_cast<std::uint32_t>(c);
      auto const sin_b = ((sb & ~swap) | (cb & swap)) ^ ((n & 2) << 30);
      auto const cos_b = ((cb & ~swap) | (sb & swap)) ^ (((n + 1) & 2) << 30);
      return {std::bit_cast<float>(sin_b), std::bit_cast<float>(cos_b)};
   }

   ////////////////////////////////////////////////////////////////////////////
   // fast_atan2: atan2(y, x), in [-pi, pi]. The arctangent of the smaller
   // over the larger of |y| and |x| comes from Abramowitz and Stegun 4.4.47
   // (error at most 1e-5 on [-1, 1]); the octant then sets the angle,
   // from sign bits, with no compare. fast_atan2(0, 0) is 0.
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_atan2(float y, float x)
   {
      constexpr float half_pi = float(pi / 2);

      float const ax = std::abs(x);
      float const ay = std::abs(y);
      float const hi = std::fmax(ax, ay);
      float const lo = std::fmin(ax, ay);
      float const z = lo / std::fmax(hi, std::numeric_limits<float>::min());
      float const z2 = z * z;
      float const a = z * (0.9998660f + z2 * (-0.3302995f + z2 * (0.1801410f
         + z2 * (-0.0851330f + z2 * 0.0208351f))));

      // Masks of all ones where |y| > |x| and where x < 0.
      auto mask = [](float v)
      {
         return std::uint32_t(std::bit_cast<std::int32_t>(v) >> 31);
      };
      auto pick = [](float a, float b, std::uint32_t m)
      {
         return std::bit_cast<float>((std::bit_cast<std::uint32_t>(a) & ~m)
            | (std::bit_cast<std::uint32_t>(b) & m));
      };
      float const steep = pick(a, half_pi - a, mask(ax - ay));
      float const side = pick(steep, float(pi) - steep, mask(x));
      return std::copysign(side, y);
   }

   ////////////////////////////////////////////////////////////////////////////
   // fast pade-approximation of the tanh function (x should be: -3 <= x <= 3)
   ////////////////////////////////////////////////////////////////////////////
   constexpr float fast_rational_tanh(float x)
   {
      return x * (27 + x*x) / (27 + 9*x*x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast tanh, exp-based. Cheaper than std::tanh, and on a desktop FPU
   // typically faster than fast_rational_tanh (branchless, no divide), at the
   // cost of the bit-level exp approximation's error.
   //
   // The argument is clamped to +/-9 first. tanh is 1.0f in float from about
   // 9 up, so the clamp changes no result, and it keeps the underlying
   // -1 + 2/(1 + exp(-2x)) from evaluating an exp that overflows: without it,
   // an argument below about -134 returns NaN. A caller that saturates a
   // driven signal reaches that easily.
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_tanh(float x)
   {
      constexpr auto lim = 9.0f;
      return fasttanh(x < -lim ? -lim : (x > lim ? lim : x));
   }

   inline float faster_tanh(float x)
   {
      constexpr auto lim = 9.0f;
      return fastertanh(x < -lim ? -lim : (x > lim ? lim : x));
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast exp
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_exp(float x)
   {
      return fastexp(x);
   }

   inline float faster_exp(float x)
   {
      return fasterexp(x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast exp Taylor series approximations (from http://www.musicdsp.org/)
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
   // Fast reciprocal. See http://tinyurl.com/lgmnsyg. We want to avoid
   // multiplication. So instead of 1.0f/val, we use this function which
   // is inaccurate, but fast substitute. It works by negating the exponent
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
   // Fast log
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_log(float x)
   {
      return fastlog(x);
   }

   inline float faster_log(float x)
   {
      return fasterlog(x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast log2
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_log2(float x)
   {
      return fastlog2(x);
   }

   inline float faster_log2(float x)
   {
      return fasterlog2(x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast pow2
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_pow2(float x)
   {
      return fastpow2(x);
   }

   inline float faster_pow2(float x)
   {
      return fasterpow2(x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast sqrt -- now just std::sqrt. Benchmarked across ARM (NEON), x86 GCC
   // and x86 MSVC: std::sqrt lowers to a single hardware sqrt instruction and
   // is the fastest option on every target -- ahead of both this function's
   // former log2/pow2 approximation and a hand-rolled NEON/SSE rsqrt + Newton
   // step -- while being exact. Kept as a named alias so call sites need not
   // change.
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_sqrt(float x)
   {
      return std::sqrt(x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast log10
   ////////////////////////////////////////////////////////////////////////////
   inline float fast_log10(float x)
   {
      return 0.301029995663981f * fast_log2(x);
   }

   inline float faster_log10(float x)
   {
      return 0.301029995663981f * faster_log2(x);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Fast pow10
   //
   // pow10(x) == pow2(x * log2(10)). We compute it directly via pow2 using the
   // exact log2(10). The generic fastpow(10, x)/fasterpow(10, x) would instead
   // scale x by their own *approximate* fastlog2(10)/fasterlog2(10), injecting
   // a systematic bias. Folding in the exact constant costs the same (a single
   // multiply) yet is markedly more accurate: vs std::pow, fast_pow10 RMSE
   // drops ~5x and faster_pow10 ~2x (see test/support/decibel.cpp).
   ////////////////////////////////////////////////////////////////////////////
   constexpr float log2_10 = 3.321928094887362f;   // log2(10)

   inline float fast_pow10(float x)
   {
      return fast_pow2(x * log2_10);
   }

   inline float faster_pow10(float x)
   {
      return faster_pow2(x * log2_10);
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

   inline bool abs_within(int a, int b, int eps)
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
