/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_INFINITY_INTERPOLATION_JULY_20_2014)
#define CYCFI_INFINITY_INTERPOLATION_JULY_20_2014

#include <q/support/base.hpp>
#include <q/utility/interpolation_primitives.hpp>
#include <q/detail/sin_table.hpp>
#include <cmath>
#include <cstddef>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // cosine: a 2-point cosine ease, sin^2(pi*mu/2), computed via the q sin
   // lookup table: mu maps directly to the first quarter cycle (phase
   // fraction mu/4), avoiding both std::cos and any radian conversion.
   // Table error (~5e-6) is negligible against the ease's own distortion.
   //
   // The other primitives are in q/utility/interpolation_primitives.hpp;
   // this one is here because it is the only one that needs the sin table.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   constexpr T cosine_interpolate(T y1, T y2, T mu)
   {
      auto const s = T(sin_lu(frac_to_phase(mu * T(0.25))));
      return linear_interpolate(y1, y2, s * s);
   }

   ////////////////////////////////////////////////////////////////////////////
   // sample_interpolation: policies for fractional (sub-sample) buffer
   // indexing. Given a buffer and a fractional index, each policy computes
   // a value between samples. The buffer's index 0 is the newest element;
   // increasing indices go back in time.
   //
   // Valid fractional index ranges:
   //
   //    2-point types (linear, cosine):           [0, size-2]
   //    4-point types (cubic, hermite, bspline):  [1, size-3]
   //
   // The 4-point types read one sample on each side of the bracketing
   // pair; indexing outside the valid range wraps around the ring buffer
   // and yields garbage.
   ////////////////////////////////////////////////////////////////////////////
   namespace sample_interpolation
   {
      ////////////////////////////////////////////////////////////////////////
      // none: zero-order hold. The fractional part of the index is
      // truncated; returns the sample at the integral index.
      ////////////////////////////////////////////////////////////////////////
      struct none
      {
         template <typename Storage, typename T>
         T operator()(Storage const& buffer, T index) const
         {
            return buffer[std::size_t(index)];
         }
      };

      ////////////////////////////////////////////////////////////////////////
      // linear: first-order (2-point) interpolation. Reproduces linear
      // ramps exactly. Worst-case error on a sinusoid with angular
      // increment w is ~w^2/8. The workhorse: cheapest type with
      // pass-through at the samples.
      ////////////////////////////////////////////////////////////////////////
      struct linear
      {
         template <typename Storage, typename T>
         T operator()(Storage const& buffer, T index) const
         {
            auto const i = std::size_t(index);
            auto const mu = index - std::floor(index);
            return linear_interpolate<T>(buffer[i], buffer[i + 1], mu);
         }
      };

      ////////////////////////////////////////////////////////////////////////
      // cosine: 2-point cosine ease. Same footprint as linear, with a
      // continuous first derivative at the segment boundaries (zero slope
      // at each sample) and no overshoot: the output stays strictly within
      // [y1, y2]. The ease distorts in-band content: it is exact only at
      // mu = 0, 0.5 and 1, so on band-limited audio it is less accurate
      // than linear. Its niche is control-signal smoothing (automation,
      // breakpoints, value noise) where pass-through, C1 continuity and
      // the no-overshoot guarantee matter more than spectral accuracy.
      //
      // The ease is computed as sin^2(pi*mu/2) via the sin lookup table:
      // mu maps directly to the first quarter cycle (phase fraction mu/4),
      // avoiding both std::cos and any radian conversion. Table error
      // (~5e-6) is negligible against the ease's inherent distortion.
      ////////////////////////////////////////////////////////////////////////
      struct cosine
      {
         template <typename Storage, typename T>
         T operator()(Storage const& buffer, T index) const
         {
            auto const i = std::size_t(index);
            auto const mu = index - std::floor(index);
            return cosine_interpolate<T>(buffer[i], buffer[i + 1], mu);
         }
      };

      ////////////////////////////////////////////////////////////////////////
      // cubic: 4-point, third-order Lagrange interpolation. Passes through
      // the samples and reproduces cubic polynomials exactly. The most
      // accurate of the 4-point types on smooth signals, but its first
      // derivative is discontinuous at segment boundaries; prefer hermite
      // for taps whose position is modulated.
      ////////////////////////////////////////////////////////////////////////
      struct cubic
      {
         template <typename Storage, typename T>
         T operator()(Storage const& buffer, T index) const
         {
            auto const i = std::size_t(index);
            auto const mu = index - std::floor(index);
            return cubic_interpolate<T>(
               buffer[i - 1], buffer[i], buffer[i + 1], buffer[i + 2], mu);
         }
      };

      ////////////////////////////////////////////////////////////////////////
      // hermite: 4-point, third-order cubic Hermite (Catmull-Rom: tangents
      // from central differences). Passes through the samples, reproduces
      // quadratics exactly, and is C1-continuous across segment boundaries
      // -- the standard choice for modulated delay-line taps.
      ////////////////////////////////////////////////////////////////////////
      struct hermite
      {
         template <typename Storage, typename T>
         T operator()(Storage const& buffer, T index) const
         {
            auto const i = std::size_t(index);
            auto const mu = index - std::floor(index);
            return hermite_interpolate<T>(
               buffer[i - 1], buffer[i], buffer[i + 1], buffer[i + 2], mu);
         }
      };

      ////////////////////////////////////////////////////////////////////////
      // bspline: 4-point cubic B-spline. A smoother, not an interpolator:
      // it does not pass through the samples (at an integer index it
      // yields (y[i-1] + 4*y[i] + y[i+1])/6), trading pass-through
      // exactness for the best high-frequency rejection and C2 continuity
      // of the cubics. It has linear precision: constants and ramps are
      // reproduced exactly. Use when the buffer holds noisy data.
      ////////////////////////////////////////////////////////////////////////
      struct bspline
      {
         template <typename Storage, typename T>
         T operator()(Storage const& buffer, T index) const
         {
            auto const i = std::size_t(index);
            auto const mu = index - std::floor(index);
            return bspline_interpolate<T>(
               buffer[i - 1], buffer[i], buffer[i + 1], buffer[i + 2], mu);
         }
      };
   }
}

#endif
