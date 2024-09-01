/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PHASE_HPP_MAY_2018)
#define CYCFI_Q_PHASE_HPP_MAY_2018

#include <q/support/base.hpp>
#include <q/support/literals.hpp>
#include <q/support/basic_concepts.hpp>
#include <infra/assert.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Type safe representation of phase: the relationship in timing between a
   // periodic signal relative to a reference periodic signal of the same
   // frequency. Phase values run from 0 to 2π,  suitable for oscillators.
   // `phase` is represented as fixed point 1.31 format where 31 bits are
   // fractional.
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
   struct phase_unit;

   struct phase : unit<std::uint32_t, phase>
   {
      using base_type = unit<std::uint32_t, phase>;
      using base_type::base_type;
      using unit_type = phase_unit;

      constexpr static auto one_cyc = int_max<std::uint32_t>();
      constexpr static auto bits = sizeof(std::uint32_t) * 8;

                                    [[deprecated("Use frac_to_phase(frac) instead.")]]
      constexpr                     phase(std::floating_point auto frac);

      constexpr                     phase();
      constexpr                     phase(frequency freq, float sps);

      constexpr static phase        begin()     { return phase{}; }
      constexpr static phase        end()       { return phase(one_cyc); }
      constexpr static phase        middle()    { return phase(one_cyc/2); }
   };

   // Free functions
   constexpr phase   frac_to_phase(std::floating_point auto frac);
   constexpr double  frac_double(phase p);
   constexpr float   frac_float(phase p);

   ////////////////////////////////////////////////////////////////////////////
   // phase_iterator: iterates over the phase with an interval specified by
   // the supplied frequency.
   ////////////////////////////////////////////////////////////////////////////
   struct phase_iterator
   {
      constexpr                     phase_iterator();
      constexpr                     phase_iterator(frequency freq, float sps);

      constexpr phase_iterator      operator++(int);
      constexpr phase_iterator&     operator++();
      constexpr phase_iterator      operator--(int);
      constexpr phase_iterator&     operator--();

      constexpr phase_iterator&     operator=(phase rhs);
      constexpr phase_iterator&     operator=(phase_iterator const& rhs) = default;

      constexpr void                set(frequency freq, float sps);

      constexpr bool                first() const;
      constexpr bool                last() const;
      constexpr phase_iterator      begin() const;
      constexpr phase_iterator      end() const;
      constexpr phase_iterator      middle() const;

      phase                         _phase, _step;
   };

   ////////////////////////////////////////////////////////////////////////////
   // one_shot_phase_iterator: A variant of the phase_iterator that does not
   // wrap around when incrementing at the end or when decremented at the
   // beginning.
   //
   // Note: Branchfree Saturating Arithmetic using
   // http://locklessinc.com/articles/sat_arithmetic/
   //
   ////////////////////////////////////////////////////////////////////////////
   struct one_shot_phase_iterator : phase_iterator
   {
      using phase_iterator::phase_iterator;
      using phase_iterator::operator=;

      constexpr one_shot_phase_iterator      operator++(int);
      constexpr one_shot_phase_iterator&     operator++();
      constexpr one_shot_phase_iterator      operator--(int);
      constexpr one_shot_phase_iterator&     operator--();
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      template <typename T>
      constexpr phase::value_type frac_phase(T frac)
      {
         CYCFI_ASSERT(frac >= 0.0,
            "Frac should be greater than or equal to 0"
         );
         return (frac >= 1.0)?
            phase::end().rep :
            pow2<T>(phase::bits) * frac;
      }
   }

   constexpr phase::phase(std::floating_point auto frac)
    : base_type{frac_to_phase(frac)}
   {}

   constexpr phase::phase()
     : base_type(0)
   {
   }

   constexpr phase::phase(frequency freq, float sps)
    : base_type((pow2<double>(bits) * as_double(freq)) / sps)
   {}

   constexpr phase frac_to_phase(std::floating_point auto  frac)
   {
      return phase{detail::frac_phase(frac), direct_unit};
   }

   constexpr double frac_double(phase p)
   {
      constexpr auto denom = pow2<double>(p.bits);
      return p.rep / denom;
   }

   constexpr float frac_float(phase p)
   {
      constexpr auto denom = pow2<float>(p.bits);
      return p.rep / denom;
   }

   constexpr phase_iterator::phase_iterator()
    : _phase{}
    , _step{}
   {}

   constexpr phase_iterator::phase_iterator(frequency freq, float sps)
    : _phase{}
    , _step{freq, sps}
   {}

   constexpr phase_iterator phase_iterator::operator++(int)
   {
      phase_iterator r = *this;
      _phase += _step;
      return r;
   }

   constexpr phase_iterator& phase_iterator::operator++()
   {
      _phase += _step;
      return *this;
   }

   constexpr phase_iterator phase_iterator::operator--(int)
   {
      phase_iterator r = *this;
      _phase -= _step;
      return r;
   }

   constexpr phase_iterator& phase_iterator::operator--()
   {
      _phase -= _step;
      return *this;
   }

   constexpr phase_iterator& phase_iterator::operator=(phase rhs)
   {
      _step = rhs;
      return *this;
   }

   constexpr void phase_iterator::set(frequency freq, float sps)
   {
      _step = {freq, sps};
   }

   constexpr bool phase_iterator::first() const
   {
      return _phase < _step;
   }

   constexpr bool phase_iterator::last() const
   {
      return (phase::end()-_phase) < _step;
   }

   constexpr phase_iterator phase_iterator::begin() const
   {
      auto r = *this;
      r._phase = phase::begin();
      return r;
   }

   constexpr phase_iterator phase_iterator::end() const
   {
      auto r = *this;
      r._phase = phase::end();
      return r;
   }

   constexpr phase_iterator phase_iterator::middle() const
   {
      auto r = *this;
      r._phase = phase::middle();
      return r;
   }

   constexpr one_shot_phase_iterator one_shot_phase_iterator::operator++(int)
   {
      one_shot_phase_iterator r = *this;
      ++(*this);
      return r;
   }

   constexpr one_shot_phase_iterator& one_shot_phase_iterator::operator++()
   {
      auto res = _phase.rep + _step.rep;
      res |= -(res < _phase.rep);
      _phase.rep = res;
      return *this;
   }

   constexpr one_shot_phase_iterator one_shot_phase_iterator::operator--(int)
   {
      one_shot_phase_iterator r = *this;
      --(*this);
      return r;
   }

   constexpr one_shot_phase_iterator& one_shot_phase_iterator::operator--()
   {
	   auto res = _phase.rep - _step.rep;
	   res &= -(res <= _phase.rep);
      _phase.rep = res;
      return *this;
   }
}

#endif
