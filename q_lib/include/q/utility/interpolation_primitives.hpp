/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_INTERPOLATION_PRIMITIVES_HPP_AUGUST_15_2026)
#define CYCFI_Q_INTERPOLATION_PRIMITIVES_HPP_AUGUST_15_2026

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // The interpolation primitives: the arithmetic behind the
   // sample_interpolation policies, without the buffer. `mu` runs from 0 at
   // y1 to 1 at y2; the 4-point forms take y0 before and y3 after that pair.
   //
   // Includes nothing, so it can sit below the detail table lookups that use
   // linear_interpolate. cosine_interpolate is in interpolation.hpp instead:
   // it needs the sin table, which is built on those lookups.
   ////////////////////////////////////////////////////////////////////////////

   // First-order, 2-point. Constants and ramps are exact.
   template <typename T>
   constexpr T linear_interpolate(T y1, T y2, T mu)
   {
      return y1 + mu * (y2 - y1);
   }

   // Third-order Lagrange, 4-point. Passes through the samples, exact on
   // cubics, first derivative discontinuous at the samples.
   template <typename T>
   constexpr T cubic_interpolate(T y0, T y1, T y2, T y3, T mu)
   {
      auto const c1 = y2 - y0/T(3) - y1/T(2) - y3/T(6);
      auto const c2 = (y0 + y2)/T(2) - y1;
      auto const c3 = (y3 - y0)/T(6) + (y1 - y2)/T(2);
      return ((c3*mu + c2)*mu + c1)*mu + y1;
   }

   // Cubic Hermite, Catmull-Rom tangents, 4-point. Passes through the
   // samples, exact on quadratics, C1 across segments.
   template <typename T>
   constexpr T hermite_interpolate(T y0, T y1, T y2, T y3, T mu)
   {
      auto const c1 = (y2 - y0) * T(0.5);
      auto const c2 = y0 - T(2.5)*y1 + T(2)*y2 - T(0.5)*y3;
      auto const c3 = (y3 - y0) * T(0.5) + (y1 - y2) * T(1.5);
      return ((c3*mu + c2)*mu + c1)*mu + y1;
   }

   // Cubic B-spline, 4-point. A smoother: does not pass through the
   // samples, buying C2 and the best HF rejection of the cubics.
   template <typename T>
   constexpr T bspline_interpolate(T y0, T y1, T y2, T y3, T mu)
   {
      auto const c0 = (y0 + T(4)*y1 + y2) / T(6);
      auto const c1 = (y2 - y0) * T(0.5);
      auto const c2 = (y0 - T(2)*y1 + y2) * T(0.5);
      auto const c3 = (y3 - y0)/T(6) + (y1 - y2)*T(0.5);
      return ((c3*mu + c2)*mu + c1)*mu + c0;
   }

   // Given three samples straddling a discrete maximum, returns the vertex
   // of the quadratic through them as an offset from y1, within [-0.5, 0.5].
   //
   // The fit assumes the shape is symmetric about its peak; if not, the bias
   // belongs to the shape and cancels in a DIFFERENCE of two positions on
   // the same feature. Returns 0 unless the samples bracket a maximum,
   // rather than extrapolating off the end. For a minimum, negate.
   template <typename T>
   constexpr T peak_offset(T y0, T y1, T y2)
   {
      auto const d = y0 - (T(2) * y1) + y2;
      return d < T(0) ? T(0.5) * (y0 - y2) / d : T(0);
   }
}

#endif
