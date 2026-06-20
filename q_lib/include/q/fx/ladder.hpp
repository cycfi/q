/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_LADDER_HPP_JUNE_20_2026)
#define CYCFI_Q_LADDER_HPP_JUNE_20_2026

#include <q/support/base.hpp>
#include <q/support/frequency.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // moog_ladder: a 4-pole (24 dB/oct) lowpass ladder filter with resonance,
   // the classic "fat" synthesizer voice.
   //
   // Four topology-preserving one-pole lowpasses in series, wrapped in a global
   // resonance feedback loop. In the default linear mode the instantaneous
   // (zero-delay) feedback is resolved analytically, so the filter is exact and
   // stays stable and zipper-free under per-sample cutoff modulation, the same
   // virtue as q::svf. See:
   //
   //    Vadim Zavalishin, "The Art of VA Filter Design".
   //    Will Pirkle, "Designing Software Synthesizer Plug-Ins in C++".
   //
   // Parametrization is exact:
   //
   //    cutoff      g = tan(pi * fc / fs)   bilinear prewarp; fc is the corner
   //                                        of each one-pole stage.
   //    resonance   r in [0, 1] -> k = 4r   feedback gain; the ladder reaches
   //                                        self-oscillation exactly at r = 1
   //                                        (k = 4). As r rises the passband
   //                                        thins, the characteristic ladder
   //                                        behavior.
   //
   // operator() returns the 24 dB/oct lowpass.
   //
   // The optional nonlinear mode runs a tanh saturation in the feedback path
   // for the saturating Moog tone. That path uses the previous output for the
   // nonlinearity (a one-sample delay in the saturation only; the four stages
   // stay TPT), which is the standard inexpensive nonlinear ladder; it shifts
   // tuning slightly at very high resonance. The linear mode has no such
   // approximation.
   ////////////////////////////////////////////////////////////////////////////
   struct moog_ladder
   {
      moog_ladder(frequency f, float sps, float reso = 0.0f, bool nonlinear = false)
       : _nonlinear(nonlinear)
      {
         config(f, sps, reso);
      }

      float operator()(float x)
      {
         auto const one_m_g = 1.0f - _G;        // = 1/(1+g)

         if (_nonlinear)
         {
            // Saturated feedback using the previous output (1-sample delay in
            // the nonlinearity only).
            auto u = x - _k * fast_tanh(_y);
            _y = stages(u);
            return _y;
         }

         // Linear: resolve the zero-delay feedback. The state seen through the
         // chain is S = G^3*S1 + G^2*S2 + G*S3 + S4, with Si = (1-G)*si.
         auto S =
            _G3 * (one_m_g * _s1) +
            _G2 * (one_m_g * _s2) +
            _G  * (one_m_g * _s3) +
                  (one_m_g * _s4);

         auto u = (x - _k * S) * _g_res;        // _g_res = 1/(1 + k*G^4)
         _y = stages(u);
         return _y;
      }

      // Retune the cutoff (cheap: one tan and a few mults).
      void cutoff(frequency f, float sps)
      {
         auto g = fast_tan(float(pi * as_double(f) / sps));
         _G = g / (1.0 + g);
         derive();
      }

      // Resonance as a normalized amount in [0, 1]; r = 1 is self-oscillation.
      void resonance(float r)
      {
         _k = 4.0f * (r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r));
         _g_res = 1.0f / (1.0f + _k * _G4);
      }

      void config(frequency f, float sps, float r)
      {
         auto g = fast_tan(float(pi * as_double(f) / sps));
         _G = g / (1.0 + g);
         _k = 4.0f * (r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r));
         derive();
      }

      void nonlinear(bool on)    { _nonlinear = on; }

      // Clear the filter state.
      moog_ladder& operator=(float y)
      {
         _s1 = _s2 = _s3 = _s4 = _y = y;
         return *this;
      }

   private:

      float stages(float in)
      {
         auto v = (in - _s1) * _G; auto y1 = v + _s1; _s1 = y1 + v;
         v = (y1 - _s2) * _G;      auto y2 = v + _s2; _s2 = y2 + v;
         v = (y2 - _s3) * _G;      auto y3 = v + _s3; _s3 = y3 + v;
         v = (y3 - _s4) * _G;      auto y4 = v + _s4; _s4 = y4 + v;
         return y4;
      }

      void derive()
      {
         _G2 = _G * _G;
         _G3 = _G2 * _G;
         _G4 = _G3 * _G;
         _g_res = 1.0f / (1.0f + _k * _G4);
      }

      bool  _nonlinear;
      float _G = 0.0f;                 // one-pole TPT gain g/(1+g)
      float _G2, _G3, _G4;             // powers, for the feedback resolution
      float _k = 0.0f;                 // feedback gain (0..4)
      float _g_res = 1.0f;             // 1/(1 + k*G^4)
      float _s1 = 0, _s2 = 0, _s3 = 0, _s4 = 0;   // one-pole states
      float _y = 0.0f;                 // last output (feedback tap)
   };
}

#endif
