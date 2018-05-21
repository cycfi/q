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
   struct phase : value<std::uint32_t, phase>
   {
      using base_type = value<std::uint32_t, phase>;
      using base_type::base_type;

      constexpr static auto one_cyc = int_max<std::uint32_t>();
      constexpr static auto bits = sizeof(std::uint32_t) * 8;

      constexpr explicit            phase(value_type val = 0);
      constexpr explicit            phase(float frac);
      constexpr explicit            phase(double frac);
      constexpr explicit            phase(frequency freq, std::uint32_t sps);

      explicit constexpr operator   float() const;
      explicit constexpr operator   double() const;

      constexpr static phase        min()    { return phase(); }
      constexpr static phase        max()    { return phase(one_cyc); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   constexpr phase::phase(value_type val)
      : base_type(val)
   {}

   constexpr phase::phase(double frac)
    : base_type(pow2<double>(bits) * frac)
   {}

   constexpr phase::phase(float frac)
    : base_type(pow2<float>(bits) * frac)
   {}

   constexpr phase::phase(frequency freq, std::uint32_t sps)
    : base_type((pow2<float>(bits) * double(freq)) / sps)
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
}}

#endif
