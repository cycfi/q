/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_TRIANGLE_OSC_HPP_DECEMBER_24_2015)
#define CYCFI_Q_TRIANGLE_OSC_HPP_DECEMBER_24_2015

#include <q/support/phase.hpp>
#include <q/utility/antialiasing.hpp>

namespace cycfi::q
{
  ////////////////////////////////////////////////////////////////////////////
   // basic triangle-wave oscillator (not bandwidth limited)
   //
   // `symmetry` is the fraction of the cycle the wave spends rising, 0.5
   // being the plain triangle. Skewing it walks the wave from a triangle
   // toward a sawtooth (or a reverse sawtooth), which is a different harmonic
   // mix: the even harmonics come up as the symmetry leaves the middle. A
   // CEM3340 specifies its own at 50 percent with a 45 to 55 percent range.
   //
   // Where phase 0 falls differs between the two oscillators here, and both
   // keep what they have always had: the basic one troughs at phase 0, and
   // the band limited one is shifted by half the rise, so it crosses zero
   // going up there, in step with a sine. Whatever the symmetry, each keeps
   // its own landmark at phase 0.
   ////////////////////////////////////////////////////////////////////////////
   struct basic_triangle_osc
   {
      constexpr            basic_triangle_osc(float symmetry = 0.5f);

      constexpr float      operator()(phase p) const;
      constexpr float      operator()(phase_iterator i) const;

      constexpr void       symmetry(float sym);
      constexpr float      symmetry() const   { return _symmetry; }

   protected:

      float                _symmetry = 0.5f;
      float                _rise = 2.0f;      // 1 / symmetry
      float                _fall = 2.0f;      // 1 / (1 - symmetry)
      phase                _shift;            // half the rise, see above
      phase                _peak;             // where it turns
   };

   ////////////////////////////////////////////////////////////////////////////
   // triangle-wave oscillator (bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct triangle_osc : basic_triangle_osc
   {
      constexpr            triangle_osc(float symmetry = 0.5f);

      constexpr float      operator()(phase p, phase dt) const;
      constexpr float      operator()(phase_iterator i) const;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   constexpr basic_triangle_osc::basic_triangle_osc(float symmetry_)
   {
      symmetry(symmetry_);
   }

   constexpr void basic_triangle_osc::symmetry(float sym)
   {
      _symmetry = sym < 0.01f ? 0.01f : (sym > 0.99f ? 0.99f : sym);
      _rise = 1.0f / _symmetry;
      _fall = 1.0f / (1.0f - _symmetry);
      _shift = frac_to_phase(_symmetry * 0.5f);
      _peak = frac_to_phase(_symmetry);
   }

   // Phase 0 is the trough, as this oscillator has always had it.
   constexpr float basic_triangle_osc::operator()(phase p) const
   {
      constexpr float x = 1.0f / phase::one_cyc;
      auto t = p.rep * x;                    // 0 to 1 through the cycle
      return p < _peak?
         (2.0f * t * _rise) - 1.0f :
         1.0f - (2.0f * (t - _symmetry) * _fall)
         ;
   }

   constexpr float basic_triangle_osc::operator()(phase_iterator i) const
   {
      return (*this)(i._phase);
   }

   constexpr triangle_osc::triangle_osc(float symmetry_)
    : basic_triangle_osc{symmetry_}
   {}

   // The two slope breaks, at the trough and at the peak, are BLAMP
   // corrected. The slope changes by rise + fall there, in half cycles, so
   // the correction scales by 1/(symmetry * (1 - symmetry)), which is the 4 a
   // plain triangle has always used.
   constexpr float triangle_osc::operator()(phase p, phase dt) const
   {
      constexpr auto end = phase::end();
      auto u = p + _shift;                   // the band limited one is
      auto scale = _rise * _fall;            // shifted; see the note above

      auto r = basic_triangle_osc::operator()(u);
      r += poly_blamp(u, dt, scale);                  // the trough
      r -= poly_blamp(u + (end - _peak), dt, scale);  // the peak
      return r;
   }

   constexpr float triangle_osc::operator()(phase_iterator i) const
   {
      return (*this)(i._phase, i._step);
   }

   // The plain triangles, ready made. They follow the definitions above
   // because a constexpr object needs its constructor defined first.
   constexpr auto basic_triangle = basic_triangle_osc{};
   constexpr auto triangle = triangle_osc{};
}

#endif
