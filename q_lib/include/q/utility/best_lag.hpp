/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_BEST_LAG_HPP_JUNE_11_2026)
#define CYCFI_Q_BEST_LAG_HPP_JUNE_11_2026

#include <cstddef>
#include <cmath>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // best_lag: exhaustive normalized cross-correlation search for the lag,
   // within [min_lag, max_lag], at which the most recent `window` samples
   // of a (newest-first) buffer best match themselves `lag` samples ago.
   //
   // Returns the lag with sub-sample precision (parabolic interpolation
   // around the correlation peak) and the normalized similarity at the
   // peak: 1 = perfect match, 0 = uncorrelated.
   //
   // The search makes no assumption of a fundamental frequency: on
   // polyphonic material it finds the best common quasi-period within the
   // range. This makes it suitable, on arbitrary material, for loop/splice
   // point selection (sustain and freeze effects, WSOLA time stretching)
   // and for grain re-anchoring (PSOLA) -- the alignment refinement that
   // keeps period-estimate bias from accumulating across grains.
   //
   // Cost is O(window * (max_lag - min_lag)): intended for occasional,
   // event-driven use (per grain, per splice), not per sample.
   //
   // The buffer needs `window + max_lag` samples of history (plus the
   // interpolation margin if the buffer interpolates fractional indices).
   ////////////////////////////////////////////////////////////////////////////

   struct lag_result
   {
      float    lag;
      float    similarity;
   };

   template <typename Buffer>
   inline lag_result best_lag(
      Buffer const& buf
    , std::size_t window
    , std::size_t min_lag
    , std::size_t max_lag
   )
   {
      auto ncc = [&](std::size_t lag) -> double
      {
         double num = 0, e1 = 0, e2 = 0;
         for (std::size_t i = 0; i != window; ++i)
         {
            double a = buf[i];
            double b = buf[i + lag];
            num += a * b;
            e1 += a * a;
            e2 += b * b;
         }
         auto den = std::sqrt(e1 * e2);
         return den > 0.0 ? num / den : 0.0;
      };

      auto best = -2.0;
      auto best_lag_ = min_lag;
      for (auto lag = min_lag; lag <= max_lag; ++lag)
      {
         auto v = ncc(lag);
         if (v > best)
         {
            best = v;
            best_lag_ = lag;
         }
      }

      // Sub-sample refinement: fit a parabola through the peak and its
      // neighbors (interior peaks only).
      auto lag = float(best_lag_);
      if (best_lag_ > min_lag && best_lag_ < max_lag)
      {
         auto c0 = ncc(best_lag_ - 1);
         auto c2 = ncc(best_lag_ + 1);
         auto d = c0 - 2.0 * best + c2;
         if (d < 0.0)
            lag += 0.5 * (c0 - c2) / d;
      }

      return {lag, float(best)};
   }
}

#endif
