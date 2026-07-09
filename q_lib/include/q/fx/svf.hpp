/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_SVF_HPP_JUNE_20_2026)
#define CYCFI_Q_SVF_HPP_JUNE_20_2026

#include <q/support/base.hpp>
#include <q/support/frequency.hpp>
#include <q/support/duration.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Modal placement: the quality factor that places a two-pole resonator
   // by its center frequency f and amplitude decay time constant tau (the
   // 1/e time of the impulse-response envelope). From the resonator
   // bandwidth relation BW = 1/(pi*tau):
   //
   //    Q = f / BW = pi * f * tau
   //
   // This is how a mode table specifies a mode in modal synthesis: excited
   // by an impulse, the filter rings at f with an envelope that decays by
   // 1/e every tau seconds. The svf and chamberlin_filter below accept
   // (f, sps, tau) placements directly.
   ////////////////////////////////////////////////////////////////////////////
   constexpr double q_from_decay(frequency f, duration tau)
   {
      return pi * as_double(f) * as_double(tau);
   }

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

      // Modal placement: a mode at f whose impulse-response envelope
      // decays by 1/e every tau (Q = pi*f*tau, see q_from_decay above).
      svf(frequency f, float sps, duration tau)
       : svf{f, sps, q_from_decay(f, tau)}
      {}

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
         _g = fast_tan(float(pi * as_double(f) / sps));
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
         _g = fast_tan(float(pi * as_double(f) / sps));
         _k = 1.0 / q;
         update();
      }

      // Re-place the mode (modal placement, see q_from_decay above).
      void config(frequency f, float sps, duration tau)
      {
         config(f, sps, q_from_decay(f, tau));
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

   ////////////////////////////////////////////////////////////////////////////
   // chamberlin_filter: a Chamberlin state-variable filter, the cheap resonant
   // filter. (reso_filter is a deprecated alias for this type.)
   //
   // Two integrators with feedback, the textbook digital SVF (Hal Chamberlin,
   // "Musical Applications of Microprocessors"). The coefficient
   // f = 2*sin(pi*fc/fs) sets the cutoff and the damping q = 1/Q sets the
   // resonance, decoupled from the cutoff; one tick yields lowpass, bandpass,
   // highpass and notch. It uses sin rather than the tan of the TPT svf above,
   // so it is a touch cheaper to retune, at the cost of the classic Chamberlin
   // trade-offs:
   //
   //  - it is stable only below fs/6 (~7.35 kHz at 44.1k). The cutoff is clamped
   //    to that limit, so a swept cutoff saturates at the top instead of blowing
   //    up. (The previous version of this filter coupled resonance into the
   //    cutoff via reso/(1 - f), which divided by zero and produced NaN when the
   //    cutoff was swept up; that is gone.)
   //  - the 2*sin coefficient warps the placement slightly versus the exact
   //    bilinear prewarp, most noticeably as the cutoff approaches fs/6.
   //
   // For an exact, full-range resonant filter use svf above; for a 4-pole ladder
   // use q::moog_ladder (q/fx/ladder.hpp). All three are stable under per-sample
   // cutoff modulation.
   ////////////////////////////////////////////////////////////////////////////
   struct chamberlin_filter
   {
      static constexpr double default_q = 0.707106781186548;  // 1/sqrt(2)

      // Stable for fc < fs/6, i.e. f < 1; clamp a touch below.
      static constexpr float max_coeff = 0.99f;

      chamberlin_filter(frequency f, float sps, double q = default_q)
       : _q(1.0f / q)
      {
         cutoff(f, sps);
      }

      // Modal placement: a mode at f whose impulse-response envelope
      // decays by 1/e every tau (Q = pi*f*tau, see q_from_decay above).
      chamberlin_filter(frequency f, float sps, duration tau)
       : chamberlin_filter{f, sps, q_from_decay(f, tau)}
      {}

      chamberlin_filter(float coeff, double q = default_q)
       : _q(1.0f / q)
      {
         cutoff(coeff);
      }

      float operator()(float s)
      {
         _lp += _f * _bp;
         _hp = s - _lp - _q * _bp;
         _bp += _f * _hp;
         return _lp;
      }

      float operator()() const   { return _lp; }

      float lowpass() const      { return _lp; }
      float bandpass() const     { return _bp; }
      float highpass() const     { return _hp; }
      float notch() const        { return _hp + _lp; }

      void cutoff(frequency f, float sps)
      {
         cutoff(2.0f * fastsin(pi * as_float(f) / sps));
      }

      // Set the coefficient f = 2*sin(pi*fc/fs) directly, clamped to the stable
      // range [0, max_coeff].
      void cutoff(float coeff)
      {
         _f = coeff < 0.0f ? 0.0f : (coeff > max_coeff ? max_coeff : coeff);
      }

      void resonance(double q)   { _q = 1.0f / q; }   // exact Q

      // Normalized resonance in [0, 1]: 0 -> Q = 0.5, approaching 1 -> self-osc.
      void normalized_resonance(float r)
      {
         auto rr = r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r);
         _q = 2.0f * (1.0f - rr);
      }

      chamberlin_filter& operator=(float y)
      {
         _lp = y; _bp = 0.0f; _hp = 0.0f;
         return *this;
      }

      float _f = 0.0f, _q;
      float _lp = 0.0f, _bp = 0.0f, _hp = 0.0f;
   };

   // Deprecated former name for chamberlin_filter.
   using reso_filter [[deprecated("renamed to chamberlin_filter")]] =
      chamberlin_filter;
}

#endif
