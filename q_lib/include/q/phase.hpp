/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_PHASE_HPP_MAY_2018)
#define CYCFI_Q_PHASE_HPP_MAY_2018

#include <q/support.hpp>
#include <q/literals.hpp>

namespace cycfi { namespace q
{
	using namespace literals;

   ////////////////////////////////////////////////////////////////////////////
   // phase: The synthesizers use fixed point 1.31 format computations where
   // 31 the bits are fractional. phase represents phase values that runs
   // from 0 to 4294967295 (0 to 2π).
   //
   // The turn, also cycle, full circle, revolution, and rotation, is a
   // complete circular movement or measure (as to return to the same point)
   // with circle or ellipse. A turn is abbreviated τ, cyc, rev, or rot
   // depending on the application. The symbol τ can also be used as a
   // mathematical constant to represent 2π radians.
   //
   //    https://en.wikipedia.org/wiki/Angular_unit
   //
   ////////////////////////////////////////////////////////////////////////////
   struct phase
   {
      using value_type = std::uint32_t;

      // One complete cycle or turn:
      constexpr static auto one_cyc = int_max<std::uint32_t>();

      constexpr explicit   phase();
      constexpr explicit   phase(double radians);
      constexpr explicit   phase(frequency freq, std::uint32_t sps);

                           template <typename T>
      constexpr explicit   phase(T numer, T denom);

      phase&               operator+=(phase rhs);
      phase&               operator-=(phase rhs);
      phase&               operator*=(phase rhs);
      phase&               operator/=(phase rhs);

      value_type           val;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   constexpr phase::phase()
      : val(0)
   {}

   constexpr phase::phase(double radians)
    : val(promote(one_cyc) * (radians / 2_pi))
   {}

   constexpr phase::phase(frequency freq, std::uint32_t sps)
    : val((promote(one_cyc) * double(freq)) / sps)
   {}

   template <typename T>
   constexpr phase::phase(T numer, T denom)
    : val((promote(one_cyc) * numer) / denom)
   {}

   inline phase& phase::operator+=(phase rhs)
   {
      val += rhs.val;
      return *this;
   }

   inline phase& phase::operator-=(phase rhs)
   {
      val -= rhs.val;
      return *this;
   }

   inline phase& phase::operator*=(phase rhs)
   {
      val *= rhs.val;
      return *this;
   }

   inline phase& phase::operator/=(phase rhs)
   {
      val /= rhs.val;
      return *this;
   }

   constexpr bool operator==(phase a, phase b) { return a.val == b.val; }
   constexpr bool operator!=(phase a, phase b) { return a.val != b.val; }
   constexpr bool operator<(phase a, phase b) { return a.val < b.val; }
   constexpr bool operator<=(phase a, phase b) { return a.val <= b.val; }
   constexpr bool operator>(phase a, phase b) { return a.val > b.val; }
   constexpr bool operator>=(phase a, phase b) { return a.val >= b.val; }

   inline phase operator+(phase a, phase b) { auto r = a; return r += b; }
   inline phase operator-(phase a, phase b) { auto r = a; return r -= b; }
   inline phase operator*(phase a, phase b) { auto r = a; return r *= b; }
   inline phase operator/(phase a, phase b) { auto r = a; return r /= b; }

}}

#endif
