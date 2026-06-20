/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_SVF_HPP_JUNE_20_2026)
#define CYCFI_Q_SVF_HPP_JUNE_20_2026

#include <q/support/base.hpp>
#include <q/support/frequency.hpp>
#include <cmath>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // svf: a topology-preserving-transform (TPT) state-variable filter.
   //
   // Derived from the continuous-time analog SVF discretized with trapezoidal
   // (bilinear) integration, with the unit-delay-free feedback resolved
   // analytically (zero-delay feedback). See:
   //
   //    Andrew Simper (Cytomic), "Solving the continuous SVF equations using
   //    trapezoidal integration and equivalent currents", 2013.
   //    https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
   //
   //    Vadim Zavalishin, "The Art of VA Filter Design".
   //
   // Why this and not an RBJ biquad for modulated filters: the two state
   // variables are integrator outputs with a consistent physical meaning, so
   // the cutoff coefficient g may be changed every sample and the filter stays
   // stable and free of zipper noise, even at high resonance and into
   // self-oscillation. A direct-form biquad, by contrast, stores past in/out
   // samples that only match the coefficients that produced them; swapping
   // coefficients under that state (as an envelope sweep does) clicks and can
   // ring or blow up.
   //
   // Parametrization is exact:
   //
   //    cutoff     g = tan(pi * fc / fs)   bilinear prewarp: the actual -3 dB /
   //                                       resonant point lands exactly at fc,
   //                                       for any fc up to Nyquist.
   //    resonance  k = 1 / Q               Q is the true quality factor:
   //                                       Q = 1/sqrt(2) ~ 0.707 is maximally
   //                                       flat (Butterworth), larger Q sharpens
   //                                       the peak, k -> 0 (Q -> inf) self
   //                                       oscillates.
   //
   // One tick yields every second-order response from the same two states:
   // lowpass, bandpass, highpass, notch, peak, and allpass. operator() runs the
   // tick and returns the lowpass; the rest are read back with the accessors.
   //
   // The filter is unconditionally stable for g > 0 and k >= 0.
   ////////////////////////////////////////////////////////////////////////////
   struct svf
   {
      static constexpr double default_q = 0.707106781186548; // 1/sqrt(2)

      svf(frequency f, float sps, double q = default_q)
      {
         config(f, sps, q);
      }

      // Run one sample. Returns the lowpass output; the other modes are
      // available from the accessors until the next call.
      float operator()(float x)
      {
         _x = x;
         auto v3 = x - _ic2eq;
         _v1 = _a1 * _ic1eq + _a2 * v3;
         _v2 = _ic2eq + _a2 * _ic1eq + _a3 * v3;
         _ic1eq = 2.0f * _v1 - _ic1eq;
         _ic2eq = 2.0f * _v2 - _ic2eq;
         return _v2;
      }

      // Mode outputs, valid after a call to operator().
      float lowpass() const    { return _v2; }
      float bandpass() const   { return _v1; }
      float highpass() const   { return _x - _k * _v1 - _v2; }
      float notch() const      { return _x - _k * _v1; }            // lp + hp
      float peak() const       { return (2.0f * _v2) - _x + (_k * _v1); } // lp - hp
      float allpass() const    { return _x - (2.0f * _k * _v1); }

      // Retune the cutoff (cheap: one tan, three mults, one divide).
      void cutoff(frequency f, float sps)
      {
         _g = std::tan(pi * as_double(f) / sps);
         update();
      }

      // Set resonance by the exact quality factor Q.
      void resonance(double q)
      {
         _k = 1.0 / q;
         update();
      }

      // Set resonance by a normalized, musical amount in [0, 1]:
      // 0 -> Q = 0.5 (heavily damped), approaching 1 -> self-oscillation.
      void normalized_resonance(float r)
      {
         _k = 2.0 * (1.0 - double(r));
         update();
      }

      void config(frequency f, float sps, double q)
      {
         _g = std::tan(pi * as_double(f) / sps);
         _k = 1.0 / q;
         update();
      }

      // Preset the filter so that it outputs the steady-state value y for a
      // constant input y (clears the resonant/transient states).
      svf& operator=(float y)
      {
         _ic1eq = 0.0f;
         _ic2eq = y;
         _x = _v2 = y;
         _v1 = 0.0f;
         return *this;
      }

   private:

      void update()
      {
         auto a1 = 1.0 / (1.0 + _g * (_g + _k));
         _a1 = a1;
         _a2 = _g * a1;
         _a3 = _g * _a2;
      }

      float _g, _k;             // prewarped cutoff, damping (1/Q)
      float _a1, _a2, _a3;      // derived coefficients
      float _ic1eq = 0.0f;      // integrator states (trapezoidal "currents")
      float _ic2eq = 0.0f;
      float _x = 0.0f;          // last input and the two integrator outputs,
      float _v1 = 0.0f;         // cached so the mode accessors are exact
      float _v2 = 0.0f;
   };
}

#endif
