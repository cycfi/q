/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_PEAK_PICKER_HPP_JULY_20_2026)
#define CYCFI_Q_PEAK_PICKER_HPP_JULY_20_2026

#include <q/fx/differentiator.hpp>
#include <q/fx/moving_average.hpp>

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // peak_info -- the picker's live state, exposed (by const reference) to the
   // qualifier chain on every sample.
   //
   //    s     current sample -- read by a self-contained qualifier that keeps
   //          its own running statistic (see peak_z_score)
   //    amp   apex amplitude -- the TRUE peak value, exact (from the first-
   //          difference's delayed sample), not an envelope estimate
   //
   // No position: the caller owns the timeline (a span between picks is a
   // difference, so a constant one-sample offset cancels), so whoever needs
   // the mark's sample index stamps it from its own clock.
   ////////////////////////////////////////////////////////////////////////////
   struct peak_info
   {
      float  s = 0.0f;
      float  amp = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // pick -- the value that flows down the qualifier chain. Two elements: a
   // flag that a local maximum was confirmed on this tick, and a const view of
   // the picker's state. A qualifier passes it on unchanged, or clears `hit`
   // to reject; it never mutates the state.
   ////////////////////////////////////////////////////////////////////////////
   struct pick
   {
      bool              hit;
      peak_info const&  info;
   };

   ////////////////////////////////////////////////////////////////////////////
   // peak_picker -- a causal, derivative-based local-maximum picker.
   //
   // The core is a first-difference sign change: while the signal rises the
   // apex is tracked; the first sample it ticks down, the PREVIOUS sample was
   // the apex. That is one sample of latency -- the least any causal picker can
   // manage -- and it yields the exact apex position and amplitude, with no
   // lagging envelope follower.
   //
   // The picker keeps no selectivity of its own: it reports EVERY local
   // maximum. Selectivity is added by composing qualifiers around it, Q style
   // -- gate(pk(s), rms(s)) -- each returning a pick. A qualifier that needs a
   // real-time reference (a level, a slope) takes it as a further argument,
   // injected by the host from a follower it already owns. See the `qualifier`
   // concept.
   //
   // Single-sided: it finds maxima of its input. For a two-family landmark
   // stream, run one picker on +s and another on -s.
   ////////////////////////////////////////////////////////////////////////////
   class peak_picker
   {
   public:

      // Advance one sample; returns { a local max confirmed this tick, the
      // picker state }.
      pick operator()(float s)
      {
         _info.s = s;
         float const prev = _diff.x;              // previous sample: the apex
         float const d = _diff(s);                // first difference: s - prev
         // A strict maximum: the derivative was positive and is now negative.
         // A flat sample (d == 0) is neither -- hold the rising state through
         // it, so a plateau top or a mid-descent flat spot is not a false max.
         bool const hit = _rising && d < 0.0f;
         if (hit)
            _info.amp = prev;                     // the apex was prev
         if (d > 0.0f)       _rising = true;
         else if (d < 0.0f)  _rising = false;
         return {hit, _info};
      }

      peak_info const&  info() const  { return _info; }

      void reset()
      {
         _info = peak_info{};
         _diff = first_difference{};
         _rising = false;
      }

   private:

      peak_info         _info{};
      first_difference  _diff;
      bool              _rising = false;   // last nonzero derivative was up
   };

   ////////////////////////////////////////////////////////////////////////////
   // qualifier -- the contract a peak_picker qualifier satisfies: a callable
   // whose first argument is a `pick` and that returns a `pick`, passing it on
   // unchanged or with `hit` cleared to reject. A qualifier may take further
   // real-time inputs after the pick (a level, a slope); the host owns the
   // follower that produces each one and injects its output at the call, so the
   // qualifier holds no follower of its own. See peak_gate.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Q, typename... T>
   concept qualifier =
      requires (Q q, pick p, T... t)
      {
         { q(p, t...) } -> std::same_as<pick>;
      };

   ////////////////////////////////////////////////////////////////////////////
   // peak_gate -- keep a peak only if its apex stands above `ratio` times a
   // reference `level` supplied at the call. The gate owns no follower: the
   // host computes the level from something it already runs and injects it, so
   // there is no duplicated envelope. The level chooses the behavior:
   //
   //    0               keep only peaks above zero -- the shallow "less
   //                    negative" maxima a single-sided picker finds in the
   //                    negative excursions of a bipolar signal are dropped
   //    peak-envelope   a bar that tracks a decaying note, so the dominant
   //                    crest passes while sub-cycle wiggles sit under it
   //    RMS             the steady level of a sustained tone; with the window
   //                    a fundamental period this gives one landmark per cycle
   //    mean of |s|     the same, on a cheaper measure (no square, no sqrt)
   //
   // `ratio` is the minimum crest factor (apex over level); higher is more
   // selective. The test is strict (apex > bar), so a level of 0 is exactly
   // "above zero."
   ////////////////////////////////////////////////////////////////////////////
   struct peak_gate
   {
      pick operator()(pick p, float level)
      {
         return {p.hit && p.info.amp > ratio * level, p.info};
      }

      float ratio;
   };

   ////////////////////////////////////////////////////////////////////////////
   // peak_min_slope -- reject a peak that rides a slowly-varying signal. The
   // host measures the signal's rise into the peak (a `slope` over a short
   // window) and injects it; the peak is kept only if that rise clears
   // `threshold`. A genuine peak climbs steeply into its apex, a slow drift
   // barely moves over the same window. Owns no follower.
   ////////////////////////////////////////////////////////////////////////////
   struct peak_min_slope
   {
      pick operator()(pick p, float slope)
      {
         return {p.hit && slope >= threshold, p.info};
      }

      float threshold;
   };

   ////////////////////////////////////////////////////////////////////////////
   // peak_z_score -- keep a peak only if its apex stands at least `threshold`
   // standard deviations above a moving mean of the signal over a trailing
   // window `lag`. The mean and spread adapt, so the bar tracks a noisy or
   // wandering baseline and pulls peaks out of noise where an envelope-relative
   // gate cannot. `influence` (0 to 1) damps how much a peak feeds back into
   // the baseline, so a run of peaks cannot raise the bar and hide the next.
   //
   // After the real-time z-score peak detector by Jean-Paul van Brakel,
   // https://stackoverflow.com/a/22640362 .
   ////////////////////////////////////////////////////////////////////////////
   struct peak_z_score
   {
      peak_z_score(float threshold, float influence, duration lag, float sps)
       : threshold{threshold}, influence{influence}
       , _mean{lag, sps}, _mean_sq{lag, sps}
      {}

      pick operator()(pick p)
      {
         float const m = _mean();
         float const sd = std::sqrt(std::max(_mean_sq() - m * m, 0.0f));
         float const bar = m + threshold * sd;
         float const s = p.info.s;
         // A sample above the bar feeds the baseline only partially, so peaks
         // do not inflate the mean and spread.
         float const f = (s > bar)
            ? influence * s + (1.0f - influence) * _prev : s;
         _mean(f);
         _mean_sq(f * f);
         _prev = f;
         return {p.hit && p.info.amp > bar, p.info};
      }

      float          threshold;
      float          influence;
      moving_average _mean;
      moving_average _mean_sq;
      float          _prev = 0.0f;
   };

   static_assert(qualifier<peak_gate, float>);
   static_assert(qualifier<peak_min_slope, float>);
   static_assert(qualifier<peak_z_score>);
}

#endif
