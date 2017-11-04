/*=============================================================================
   Copyright (c) 2014-2017 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SYNTH_BASE_HPP_NOVEMBER_4_2017)
#define CYCFI_Q_SYNTH_BASE_HPP_NOVEMBER_4_2017

#include <q/fixed_point.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // The synthesizers use fixed point 0.32 format computations where all
   // the bits are fractional and represents phase values that runs from
   // 0 to uint32_max (0 to 2pi).
   ////////////////////////////////////////////////////////////////////////////
   using phase_t = fixed_point<uint32_t, 32>;

   ////////////////////////////////////////////////////////////////////////////
   // osc_freq: given frequency (freq) and samples per second (sps),
   // calculate the fixed point frequency that the phase accumulator
   // (see below) requires.
   ////////////////////////////////////////////////////////////////////////////
   constexpr phase_t osc_freq(double freq, uint32_t sps)
   {
      return phase_t{freq / sps};
   }

   ////////////////////////////////////////////////////////////////////////////
   // osc_period: given period and samples per second (sps),
   // calculate the fixed point frequency that the phase accumulator
   // (see below) requires.
   ////////////////////////////////////////////////////////////////////////////
   constexpr phase_t osc_period(double period, uint32_t sps)
   {
      return phase_t{1.0} / (sps * period);
   }

   ////////////////////////////////////////////////////////////////////////////
   // osc_period: given period in terms of number of samples,
   // calculate the fixed point frequency that the phase accumulator
   // (see below) requires. Argument samples can be fractional.
   ////////////////////////////////////////////////////////////////////////////
   constexpr phase_t osc_period(double samples)
   {
      return phase_t{1.0} / samples;
   }

   ////////////////////////////////////////////////////////////////////////////
   // osc_phase: given phase (in radians), calculate the fixed point phase
   // that the phase accumulator (see below) requires. phase uses fixed
   // point 0.32 format and runs from 0 to uint32_max (0 to 2pi).
   ////////////////////////////////////////////////////////////////////////////
   constexpr phase_t osc_phase(double phase)
   {
      return phase_t{1.0} * (phase / _2pi);
   }

   ////////////////////////////////////////////////////////////////////////////
   // synth_base
   ////////////////////////////////////////////////////////////////////////////
   template <typename Derived>
   class synth_base
   {
   public:

      phase_t           operator()();

      phase_t           freq() const;
      phase_t           offset() const;

      bool              is_phase_start() const;
      phase_t           phase() const;
      void              phase(phase_t phase);

      synth_base&       operator++();
      synth_base        operator++(int);
      synth_base&       operator--();
      synth_base        operator--(int);

      Derived&          derived();
      Derived const&    derived() const;

   private:

      phase_t           _phase;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Derived>
   inline phase_t synth_base<Derived>::operator()()
   {
      auto copy = (*this)++;
      return offset() + copy.phase();
   }

   template <typename Derived>
   inline phase_t synth_base<Derived>::freq() const
   {
      return derived().freq();
   }

   template <typename Derived>
   inline phase_t synth_base<Derived>::offset() const
   {
      return derived().offset();
   }

   template <typename Derived>
   inline bool synth_base<Derived>::is_phase_start() const
   {
      return _phase < freq();
   }

   template <typename Derived>
   inline phase_t synth_base<Derived>::phase() const
   {
      return _phase;
   }

   template <typename Derived>
   inline void synth_base<Derived>::phase(phase_t phase)
   {
      _phase = phase;
   }

   template <typename Derived>
   inline synth_base<Derived>& synth_base<Derived>::operator++()
   {
      _phase += freq();
      return *this;
   }

   template <typename Derived>
   inline synth_base<Derived> synth_base<Derived>::operator++(int)
   {
      auto r = *this;
      _phase += freq();
      return r;
   }

   template <typename Derived>
   inline synth_base<Derived>& synth_base<Derived>::operator--()
   {
      _phase -= freq();
      return *this;
   }

   template <typename Derived>
   inline synth_base<Derived> synth_base<Derived>::operator--(int)
   {
      auto r = *this;
      _phase += freq();
      return r;
   }

   template <typename Derived>
   inline Derived& synth_base<Derived>::derived()
   {
      return *static_cast<Derived*>(this);
   }

   template <typename Derived>
   inline Derived const& synth_base<Derived>::derived() const
   {
      return *static_cast<Derived const*>(this);
   }
}}

#endif
