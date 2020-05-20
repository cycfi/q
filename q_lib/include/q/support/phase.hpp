/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PHASE_HPP_MAY_2018)
#define CYCFI_Q_PHASE_HPP_MAY_2018

#include <q/support/base.hpp>
#include <q/support/literals.hpp>
#include <infra/assert.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // phase: The synthesizers use fixed point 1.31 format computations where
   // 31 bits are fractional. phase represents phase values that run from 0
   // to 4294967295 (0 to 2π) suitable for oscillators.
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
      constexpr                     phase(frequency freq, std::uint32_t sps);

      constexpr explicit operator   float() const;
      constexpr explicit operator   double() const;

      constexpr static phase        min()    { return phase(); }
      constexpr static phase        max()    { return phase(one_cyc); }
   };

   ////////////////////////////////////////////////////////////////////////////
   struct phase_iterator
   {
      constexpr                     phase_iterator();
      constexpr                     phase_iterator(frequency freq, std::uint32_t sps);

      constexpr phase_iterator      operator++(int);
      constexpr phase_iterator&     operator++();
      constexpr phase_iterator      operator--(int);
      constexpr phase_iterator&     operator--();

      constexpr phase_iterator&     operator=(phase rhs);
      constexpr phase_iterator&     operator=(phase_iterator const& rhs) = default;

      constexpr void                set(frequency freq, std::uint32_t sps);

      phase                         _phase, _incr;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   constexpr phase::phase(value_type val)
      : base_type(val)
   {}

   namespace detail
   {
      constexpr phase::value_type frac_phase(double frac)
      {
         CYCFI_ASSERT(frac >= 0.0,
            "Frac should be greater than 0"
         );
         return (frac >= 1.0)?
            phase::max().rep :
            pow2<double>(phase::bits) * frac;
      }

      constexpr phase::value_type frac_phase(float frac)
      {
         CYCFI_ASSERT(frac >= 0.0f,
            "Frac should be greater than 0"
         );
         return (frac >= 1.0f)?
            phase::max().rep :
            pow2<float>(phase::bits) * frac;
      }
   }

   constexpr phase::phase(double frac)
    : base_type(detail::frac_phase(frac))
   {}

   constexpr phase::phase(float frac)
    : base_type(detail::frac_phase(frac))
   {}

   constexpr phase::phase(frequency freq, std::uint32_t sps)
    : base_type((pow2<double>(bits) * double(freq)) / sps)
   {}

   constexpr phase::operator float() const
   {
      constexpr auto denom = pow2<float>(bits);
      return rep / denom;
   }

   constexpr phase::operator double() const
   {
      constexpr auto denom = pow2<double>(bits);
      return rep / denom;
   }

   constexpr phase_iterator::phase_iterator()
    : _phase()
    , _incr()
   {}

   constexpr phase_iterator::phase_iterator(frequency freq, std::uint32_t sps)
    : _phase()
    , _incr(freq, sps)
   {}

   constexpr phase_iterator phase_iterator::operator++(int)
   {
      phase_iterator r = *this;
      _phase += _incr;
      return r;
   }

   constexpr phase_iterator& phase_iterator::operator++()
   {
      _phase += _incr;
      return *this;
   }

   constexpr phase_iterator phase_iterator::operator--(int)
   {
      phase_iterator r = *this;
      _phase -= _incr;
      return r;
   }

   constexpr phase_iterator& phase_iterator::operator--()
   {
      _phase -= _incr;
      return *this;
   }

   constexpr phase_iterator& phase_iterator::operator=(phase rhs)
   {
      _incr = rhs;
      return *this;
   }

   constexpr void phase_iterator::set(frequency freq, std::uint32_t sps)
   {
      _incr = { freq, sps };
   }
}

#endif
