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

      constexpr static auto one_cyc = int_max<std::uint32_t>();
      constexpr static auto bits = sizeof(std::uint32_t) * 8;

      constexpr explicit            phase(value_type val = 0);
      constexpr explicit            phase(double frac);
      constexpr explicit            phase(frequency freq, std::uint32_t sps);

                                    template <typename T>
      explicit constexpr            phase(T numer, T denom);

      explicit constexpr operator   float() const;
      explicit constexpr operator   double() const;

      constexpr phase&              operator+=(phase rhs);
      constexpr phase&              operator-=(phase rhs);
      constexpr phase&              operator*=(phase rhs);
      constexpr phase&              operator/=(phase rhs);

      constexpr static phase        min()   { return phase(); }
      constexpr static phase        max()     { return phase(one_cyc); }

      value_type                    val;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   constexpr phase::phase(value_type val)
      : val(val)
   {}

   constexpr phase::operator float() const
   {
      constexpr auto denom = pow2<float>(bits);
      return val / denom;
   }

   constexpr phase::operator double() const
   {
      constexpr auto denom = pow2<double>(bits);
      return val / denom;
   }

   constexpr phase::phase(double frac)
    : val(promote(one_cyc) * frac)
   {}

   constexpr phase::phase(frequency freq, std::uint32_t sps)
    : val((promote(one_cyc) * double(freq)) / sps)
   {}

   template <typename T>
   constexpr phase::phase(T numer, T denom)
    : val((promote(one_cyc) * numer) / denom)
   {}

   constexpr phase& phase::operator+=(phase rhs)
   {
      val += rhs.val;
      return *this;
   }

   constexpr phase& phase::operator-=(phase rhs)
   {
      val -= rhs.val;
      return *this;
   }

   constexpr phase& phase::operator*=(phase rhs)
   {
      val *= rhs.val;
      return *this;
   }

   constexpr phase& phase::operator/=(phase rhs)
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

   constexpr phase operator+(phase a, phase b) { auto r = a; return r += b; }
   constexpr phase operator-(phase a, phase b) { auto r = a; return r -= b; }
   constexpr phase operator*(phase a, phase b) { auto r = a; return r *= b; }
   constexpr phase operator/(phase a, phase b) { auto r = a; return r /= b; }

   template <typename A>
   constexpr phase operator+(A a, phase b)
   {
      return phase(phase::value_type(a + b.val));
   }

   template <typename A>
   constexpr phase operator-(A a, phase b)
   {
      return phase(phase::value_type(a - b.val));
   }

   template <typename A>
   constexpr phase operator*(A a, phase b)
   {
      return phase(phase::value_type(a * b.val));
   }

   template <typename A>
   constexpr phase operator/(A a, phase b)
   {
      return phase(phase::value_type(a / b.val));
   }

   template <typename B>
   constexpr phase operator+(phase a, B b)
   {
      return phase(phase::value_type(a.val + b));
   }

   template <typename B>
   constexpr phase operator-(phase a, B b)
   {
      return phase(phase::value_type(a.val - b));
   }

   template <typename B>
   constexpr phase operator*(phase a, B b)
   {
      return phase(phase::value_type(a.val * b));
   }

   template <typename B>
   constexpr phase operator/(phase a, B b)
   {
      return phase(phase::value_type(a.val / b));
   }
}}

#endif
