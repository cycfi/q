/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_ANALOG_OSC_HPP_SEPTEMBER_28_2026)
#define CYCFI_Q_ANALOG_OSC_HPP_SEPTEMBER_28_2026

#include <q/synth/va/saw_osc.hpp>
#include <q/synth/va/pulse_osc.hpp>
#include <q/synth/va/triangle_osc.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // analog_osc: one oscillator core with its waveforms available at once,
   // the way a voltage controlled oscillator chip gives them. A CEM3340 and
   // its kin run a single ramp and derive the sawtooth, the triangle and the
   // pulse from it, and a patch mixes the ones it wants, so a voice built
   // that way needs them all from one phase.
   //
   // It is the library's three virtual analog oscillators in one object,
   // sharing the caller's phase: `saw`, `pulse` and `triangle` name what
   // `operator()` would otherwise have to choose between. The controls come
   // with them, `width` from the pulse and `symmetry` from the triangle.
   //
   // Not here yet, and wanted by a Prophet voice: hard and soft sync, and the
   // finite reset time that rounds a real ramp's corner as the frequency
   // rises (the CEM3340 data sheet's 570 uA charge limit).
   ////////////////////////////////////////////////////////////////////////////
   struct analog_osc : saw_osc, pulse_osc, triangle_osc
   {
      constexpr            analog_osc(
                              float width = 0.5f, float symmetry = 0.5f
                           );

      constexpr float      saw(phase p, phase dt) const;
      constexpr float      pulse(phase p, phase dt) const;
      constexpr float      triangle(phase p, phase dt) const;

      constexpr float      saw(phase_iterator i) const;
      constexpr float      pulse(phase_iterator i) const;
      constexpr float      triangle(phase_iterator i) const;

      using pulse_osc::width;
      using triangle_osc::symmetry;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   constexpr analog_osc::analog_osc(float width_, float symmetry_)
    : pulse_osc{width_}
    , triangle_osc{symmetry_}
   {}

   constexpr float analog_osc::saw(phase p, phase dt) const
   {
      return saw_osc::operator()(p, dt);
   }

   constexpr float analog_osc::pulse(phase p, phase dt) const
   {
      return pulse_osc::operator()(p, dt);
   }

   constexpr float analog_osc::triangle(phase p, phase dt) const
   {
      return triangle_osc::operator()(p, dt);
   }

   constexpr float analog_osc::saw(phase_iterator i) const
   {
      return saw_osc::operator()(i);
   }

   constexpr float analog_osc::pulse(phase_iterator i) const
   {
      return pulse_osc::operator()(i);
   }

   constexpr float analog_osc::triangle(phase_iterator i) const
   {
      return triangle_osc::operator()(i);
   }
}

#endif
