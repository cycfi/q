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
   // clip: a hard clipper (brick-wall limiter). Passes the signal through
   // unchanged within [-max, +max] and flattens it at the rails beyond. This is
   // the harshest saturation -- the hard corner at +/-max injects strong high
   // harmonics -- but it is branch-only and constexpr, with no arithmetic.
   //
   //    max: the rail, given as a linear amplitude (default 1.0) or a decibel
   //         level (converted to linear at construction).
   ////////////////////////////////////////////////////////////////////////////
   struct clip
   {
      constexpr clip(decibel max)
       : _max(lin_float(max))
      {}

      constexpr clip(float max = 1.0f)
       : _max(max)
      {}

      constexpr float operator()(float s) const
      {
         return (s > _max) ? _max : (s < -_max) ? -_max : s;
      }

      float _max;
   };

   ////////////////////////////////////////////////////////////////////////////
   // soft_clip: a cubic soft clipper (the classic 1.5*s - 0.5*s^3 waveshaper).
   // It hard-clips to the rail first (via clip), then applies the cubic, which
   // rounds the knee for a gentler, more musical saturation than a hard clip
   // (it favors lower-order harmonics). Designed for the unit rail (the default
   // max = 1.0): the cubic is only well behaved on [-1, +1], so inputs past the
   // rail pin flat at +/-1 (its slope there is 0). Output is in [-1, +1]; the
   // slope at 0 is 1.5, a slight gain near zero.
   ////////////////////////////////////////////////////////////////////////////
   struct soft_clip : clip
   {
      constexpr float operator()(float s) const
      {
         s = clip::operator()(s);
         return 1.5 * s - 0.5 * s * s * s;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // soft_clip2: a smooth tanh-shaped saturator built on the fast rational
   // (Pade) tanh, fast_rational_tanh. Where soft_clip's cubic hard-clips before
   // shaping -- so anything past the knee pins flat at +/-1 -- this rolls off
   // gradually across the whole knee, so louder inputs stay ordered
   // (distinguishable) instead of being crushed onto a single flat rail.
   //
   // The Pade approximation is only valid for |s| <= 3 (it diverges beyond), so
   // the input is clamped to that range; s = +/-3 maps exactly to +/-1. Output is
   // in [-1, +1] with unit slope at 0 (no gain or loss near zero).
   //
   // Cost: no transcendental -- a few multiplies, one divide, and the clamp.
   // Cheaper than std::tanh and the cubic soft_clip. The exp-based fasttanh is
   // branchless and can be cheaper still on a desktop FPU, but it trades the
   // divide for type-punning, so the ranking can flip on targets without fast
   // float division. See test/benchmark/soft_clip_bench.cpp for the measured
   // comparison.
   ////////////////////////////////////////////////////////////////////////////
   struct soft_clip2
   {
      constexpr float operator()(float s) const
      {
         s = (s > 3.0f)? 3.0f : (s < -3.0f)? -3.0f : s;
         return fast_rational_tanh(s);
      }
   };
}

#endif
