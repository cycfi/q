/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_DELTA_GATE_HPP_AUGUST_23_2026)
#define CYCFI_Q_DELTA_GATE_HPP_AUGUST_23_2026

#include <q/support/base.hpp>
#include <q/support/decibel.hpp>
#include <q/fx/sample_hold.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // delta_gate: true when a signal stands `ratio` above its own value about
   // `d` ago (see sample_hold: the lookback runs d to 1.5d). A ratio, so it
   // reads a level in dB terms and is given one as a decibel or a plain
   // factor. A signal rising out of silence fires it: any level is a rise
   // against zero.
   //
   // delta_gate_bipolar fires on a change either way: `ratio` above or
   // 1/ratio below the reference. A ratio both ways needs both sides to
   // be positive; a zero on either side (no reading) keeps the gate shut.
   ////////////////////////////////////////////////////////////////////////////
   struct delta_gate
   {
                     delta_gate(float ratio, duration d, float sps);
                     delta_gate(decibel ratio, duration d, float sps);

      bool           operator()(float s);
      bool           operator()() const;
      float          then() const;

      sample_hold    _then;
      float          _ratio;
      bool           y = false;
   };

   struct delta_gate_bipolar
   {
                     delta_gate_bipolar(
                        float ratio, duration d, float sps);
                     delta_gate_bipolar(
                        decibel ratio, duration d, float sps);

      bool           operator()(float s);
      bool           operator()() const;
      float          then() const;

      sample_hold    _then;
      float          _up;
      float          _down;
      bool           y = false;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline delta_gate::delta_gate(float ratio, duration d, float sps)
    : _then{d, sps}
    , _ratio{ratio}
   {}

   inline delta_gate::delta_gate(decibel ratio, duration d, float sps)
    : delta_gate{lin_float(ratio), d, sps}
   {}

   inline bool delta_gate::operator()(float s)
   {
      y = s > _then(s) * _ratio;
      return y;
   }

   inline bool delta_gate::operator()() const
   {
      return y;
   }

   // The reference: the value the next sample is compared with.
   inline float delta_gate::then() const
   {
      return _then();
   }

   inline delta_gate_bipolar::delta_gate_bipolar(
      float ratio, duration d, float sps)
    : _then{d, sps}
    , _up{ratio}
    , _down{1.0f / ratio}
   {}

   inline delta_gate_bipolar::delta_gate_bipolar(
      decibel ratio, duration d, float sps)
    : delta_gate_bipolar{lin_float(ratio), d, sps}
   {}

   inline bool delta_gate_bipolar::operator()(float s)
   {
      float const then = _then(s);
      y = then > 0.0f && s > 0.0f
         && (s > then * _up || s < then * _down);
      return y;
   }

   inline bool delta_gate_bipolar::operator()() const
   {
      return y;
   }

   inline float delta_gate_bipolar::then() const
   {
      return _then();
   }
}

#endif
