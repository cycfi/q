/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PULSE_OSC_HPP_DECEMBER_24_2015)
#define CYCFI_Q_PULSE_OSC_HPP_DECEMBER_24_2015

#include <q/support/phase.hpp>
#include <q/utility/antialiasing.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // basic pulse oscillator (not bandwidth limited).
   ////////////////////////////////////////////////////////////////////////////
   struct basic_pulse_osc
   {
      constexpr basic_pulse_osc(float width = 0.5)
       : _shift(frac_to_phase(width))
      {}

      constexpr void width(float width)
      {
         _shift = frac_to_phase(width);
      }

      constexpr float operator()(phase p) const
      {
         return p < _shift ? 1.0f : -1.0f;
      }

      constexpr float operator()(phase_iterator i) const
      {
         return (*this)(i._phase);
      }

      phase _shift;
   };

   ////////////////////////////////////////////////////////////////////////////
   // pulse oscillator (bandwidth limited).
   ////////////////////////////////////////////////////////////////////////////
   struct pulse_osc : basic_pulse_osc
   {
      constexpr pulse_osc(float width = 0.5)
       : basic_pulse_osc(width)
      {}

      constexpr float operator()(phase p, phase dt) const
      {
         constexpr auto end = phase::end();
         auto r = p < _shift ? 1.0f : -1.0f;

         // Correct rising discontinuity
         r += poly_blep(p, dt);

         // Correct falling discontinuity
         r -= poly_blep(p + (end - _shift), dt);

         return r;
      }

      constexpr float operator()(phase_iterator i) const
      {
         return (*this)(i._phase, i._step);
      }
   };
}

#endif
