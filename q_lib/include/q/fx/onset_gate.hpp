/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_ONSET_GATE_HPP_JANUARY_17_2021)
#define CYCFI_Q_ONSET_GATE_HPP_JANUARY_17_2021

#include <q/fx/noise_gate.hpp>
#include <q/fx/differentiator.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // onset_gate is a specialization of noise_gate that adds a slope guard for
   // rejecting slow attacks. It has two independent thresholds:
   //
   //    onset_threshold  -- an absolute LEVEL (as in noise_gate)
   //    slope_threshold  -- the required envelope RISE over attack_width
   //                        (see differentiator.hpp)
   //
   // The gate opens if the signal is EITHER unambiguously loud OR rising fast:
   //
   //    open if  env > onset_threshold  OR  slope(env) > slope_threshold
   //
   // Above onset_threshold the level alone opens the gate at once -- a loud
   // note need not wait for (and clip) its attack. Below it, the slope guard
   // governs: only a fast rise passes, so slow-moving signals (noise, bleed)
   // are rejected. Release is unchanged: the gate closes when the level drops
   // below release_threshold.
   //
   // The constructor parameters `attack_width` and `sps` specify the slope's
   // delta time.
   ////////////////////////////////////////////////////////////////////////////
   struct onset_gate : noise_gate
   {
                  onset_gate(
                     decibel onset_threshold
                   , decibel slope_threshold
                   , decibel release_threshold
                   , duration attack_width
                   , float sps
                  );

      using noise_gate::operator();

      bool        operator()(float env);

      void        slope_threshold(decibel t);
      float       slope_threshold() const;

   private:

      float       _slope_threshold;
      slope       _slope;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   inline onset_gate::onset_gate(
      decibel onset_threshold
    , decibel slope_threshold
    , decibel release_threshold
    , duration attack_width
    , float sps
   )
    : noise_gate{onset_threshold, release_threshold}
    , _slope_threshold{lin_float(slope_threshold)}
    , _slope{attack_width, sps}
   {
   }

   inline bool onset_gate::operator()(float env)
   {
      if (!_state && (env > _onset_threshold || _slope(env) > _slope_threshold))
         _state = 1;
      else if (_state && env < _release_threshold)
         _state = 0;
      return _state;
   }

   inline void onset_gate::slope_threshold(decibel t)
   {
      _slope_threshold = lin_float(t);
   }

   inline float onset_gate::slope_threshold() const
   {
      return _slope_threshold;
   }
}

#endif
