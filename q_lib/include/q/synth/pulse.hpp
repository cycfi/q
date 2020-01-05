/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PULSE_HPP_DECEMBER_24_2015)
#define CYCFI_Q_PULSE_HPP_DECEMBER_24_2015

#include <q/support/phase.hpp>
#include <q/utility/antialiasing.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // basic pulse synthesizer (not bandwidth limited).
   ////////////////////////////////////////////////////////////////////////////
   struct basic_pulse_synth
   {
      constexpr basic_pulse_synth(float width = 0.5)
       : _shift(phase(width))
      {}

      constexpr void width(float width)
      {
         _shift = phase(width);
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

   constexpr auto basic_pulse = basic_pulse_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // pulse synthesizer (bandwidth limited).
   ////////////////////////////////////////////////////////////////////////////
   struct pulse_synth : basic_pulse_synth
   {
      constexpr pulse_synth(float width = 0.5)
       : basic_pulse_synth(width)
      {}

      constexpr float operator()(phase p, phase dt) const
      {
         constexpr auto end = phase::max();
         auto r = p < _shift ? 1.0f : -1.0f;

         // Correct rising discontinuity
         r += poly_blep(p, dt);

         // Correct falling discontinuity
         r -= poly_blep(p + (end - _shift), dt);

         return r;
      }

      constexpr float operator()(phase_iterator i) const
      {
         return (*this)(i._phase, i._incr);
      }
   };

   constexpr auto pulse = pulse_synth{};
}

#endif
