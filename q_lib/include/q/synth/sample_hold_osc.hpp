/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_SAMPLE_HOLD_OSC_HPP_SEPTEMBER_16_2026)
#define CYCFI_Q_SAMPLE_HOLD_OSC_HPP_SEPTEMBER_16_2026

#include <q/support/phase.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // sample_hold_osc: a stepped waveform. It samples the signal passed in,
   // `s`, each time the phase wraps to a new cycle (or is reset), and holds
   // it until the next. Any signal will do; white noise gives the random
   // wave of an LFO, as in the DX7. Unlike the plain oscillators it keeps
   // state: the held value and the last phase.
   ////////////////////////////////////////////////////////////////////////////
   struct sample_hold_osc
   {
      constexpr float   operator()(phase p, float s);
      constexpr float   operator()(phase_iterator i, float s);

   private:

      float             _held = 0.0f;
      phase             _prev;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   constexpr float sample_hold_osc::operator()(phase p, float s)
   {
      if (p < _prev)
         _held = s;
      _prev = p;
      return _held;
   }

   constexpr float sample_hold_osc::operator()(phase_iterator i, float s)
   {
      return (*this)(i._phase, s);
   }
}

#endif
