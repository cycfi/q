/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_WAVE_CLIP_DECEMBER_24_2015)
#define CYCFI_Q_WAVE_CLIP_DECEMBER_24_2015

#include <q/support/base.hpp>
#include <q/support/decibel.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // hard_clip: a hard clipper (brick-wall limiter). Passes the signal through
   // unchanged within [-max, +max] and flattens it at the rails beyond. This is
   // the harshest saturation -- the hard corner at +/-max injects strong high
   // harmonics -- but it is branch-only and constexpr, with no arithmetic.
   //
   //    max: the rail, given as a linear amplitude (default 1.0) or a decibel
   //         level (converted to linear at construction).
   ////////////////////////////////////////////////////////////////////////////
   struct hard_clip
   {
      constexpr hard_clip(decibel max)
       : _max(lin_float(max))
      {}

      constexpr hard_clip(float max = 1.0f)
       : _max(max)
      {}

      constexpr float operator()(float s) const
      {
         return (s > _max) ? _max : (s < -_max) ? -_max : s;
      }

      float _max;
   };

   // Deprecated former name for hard_clip.
   using clip [[deprecated("renamed to hard_clip")]] = hard_clip;

   ////////////////////////////////////////////////////////////////////////////
   // cubic_clip: a cubic soft clipper (the classic 1.5*s - 0.5*s^3 waveshaper).
   // It hard-clips to the rail first (via hard_clip), then applies the cubic,
   // which rounds the knee for a gentler, more musical saturation than a hard
   // clip (it favors lower-order harmonics). Designed for the unit rail (the
   // default max = 1.0): the cubic is only well behaved on [-1, +1], so inputs
   // past the rail pin flat at +/-1 (its slope there is 0). Output is in
   // [-1, +1]; the slope at 0 is 1.5, a slight gain near zero. (This cubic is a
   // polynomial fit of a quarter sine -- very close to sin(pi/2 * s) but cheaper
   // and constexpr.)
   ////////////////////////////////////////////////////////////////////////////
   struct cubic_clip : hard_clip
   {
      constexpr float operator()(float s) const
      {
         s = hard_clip::operator()(s);
         return 1.5 * s - 0.5 * s * s * s;
      }
   };

   // Deprecated former name for cubic_clip.
   using soft_clip [[deprecated("renamed to cubic_clip")]] = cubic_clip;

   ////////////////////////////////////////////////////////////////////////////
   // tanh_clip: a smooth tanh-shaped soft clipper built on the exp-based
   // fast_tanh. tanh saturates over all reals, so no input clamp is needed
   // (unlike cubic_clip's +/-1 rail): louder inputs roll off
   // gradually and stay ordered (distinguishable) rather than pinning flat.
   // On a desktop FPU it is the cheapest soft clip (branchless, no divide);
   // the trade-offs are fast_tanh's bit-level approximation error and that it
   // is not constexpr. See test/benchmark/soft_clip_bench.cpp for the
   // measured comparison.
   //
   //    max: the rail, given as a linear amplitude (default 1.0) or a decibel
   //         level (converted to linear at construction), same as hard_clip.
   //         The curve is max * fast_tanh(s / max): output in [-max, +max],
   //         unit slope at 0.
   ////////////////////////////////////////////////////////////////////////////
   struct tanh_clip
   {
      tanh_clip(decibel max)
       : _max(lin_float(max))
       , _inv_max(1.0f / _max)
      {}

      tanh_clip(float max = 1.0f)
       : _max(max)
       , _inv_max(1.0f / max)
      {}

      float operator()(float s) const
      {
         return _max * fast_tanh(s * _inv_max);
      }

      float _max, _inv_max;
   };
}

#endif
