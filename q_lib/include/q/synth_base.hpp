/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SYNTH_BASE_HPP_NOVEMBER_4_2017)
#define CYCFI_Q_SYNTH_BASE_HPP_NOVEMBER_4_2017

#include <q/support.hpp>
#include <q/literals.hpp>

namespace cycfi { namespace q
{
	using namespace literals;

   ////////////////////////////////////////////////////////////////////////////
   // phase: The synthesizers use fixed point 1.31 format computations where
   // 31 the bits are fractional. phase represents phase values that runs from
   // 0 to 4294967295 (0 to 2π).
   ////////////////////////////////////////////////////////////////////////////

   using phase_t = std::uint32_t;

   // The turn, also cycle, full circle, revolution, and rotation, is complete
   // circular movement or measure (as to return to the same point) with circle
   // or ellipse. A turn is abbreviated τ, cyc, rev, or rot depending on the
   // application. The symbol τ can also be used as a mathematical constant to
   // represent 2π radians. (https://en.wikipedia.org/wiki/Angular_unit)

   // One complete cycle or turn:
   constexpr phase_t one_cyc = int_max<phase_t>();

   namespace phase
   {
      /////////////////////////////////////////////////////////////////////////
      // phase: given number, calculate the fixed point frequency that the
      // phase accumulator (see below) requires.
      /////////////////////////////////////////////////////////////////////////
      template <typename T>
      constexpr phase_t phase(T n)
      {
         return one_cyc * n;
      }

      /////////////////////////////////////////////////////////////////////////
      // frac: given numerator and denominator, calculate the fixed point
      // frequency that the phase accumulator (see below) requires.
      /////////////////////////////////////////////////////////////////////////
      template <typename T>
      constexpr phase_t frac(T numer, T denom)
      {
         return (promote(one_cyc) * numer) / denom;
      }

      /////////////////////////////////////////////////////////////////////////
      // freq: given frequency (freq) and samples per second (sps),
      // calculate the fixed point frequency that the phase accumulator
      // (see below) requires.
      /////////////////////////////////////////////////////////////////////////
      template <typename T>
      constexpr phase_t freq(T freq, std::uint32_t sps)
      {
         return (promote(one_cyc) * freq) / sps;
      }

      /////////////////////////////////////////////////////////////////////////
      // angle: given phase (in radians), calculate the fixed point phase
      // that the phase accumulator (see below) requires.
      /////////////////////////////////////////////////////////////////////////
      template <typename T>
      constexpr phase_t angle(T phase)
      {
         return promote(one_cyc) * (phase / 2_pi);
      }

      /////////////////////////////////////////////////////////////////////////
      // period: given period and samples per second (sps),
      // calculate the fixed point frequency that the phase accumulator
      // (see below) requires.
      /////////////////////////////////////////////////////////////////////////
      template <typename T>
      constexpr phase_t period(T period, std::uint32_t sps)
      {
         return one_cyc / (sps * period);
      }

      /////////////////////////////////////////////////////////////////////////
      // period: given period in terms of number of samples,
      // calculate the fixed point frequency that the phase accumulator
      // (see below) requires. Argument samples can be fractional.
      /////////////////////////////////////////////////////////////////////////
      template <typename T>
      constexpr phase_t period(T samples)
      {
         return one_cyc / samples;
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // synth_base
   ////////////////////////////////////////////////////////////////////////////
   template <typename Freq, typename Shift>
   struct synth_base
   {
      synth_base(Freq freq_, Shift shift_)
       : freq(freq_)
       , shift(shift_)
      {}

      phase_t     next();
      phase_t     get() const;
      bool        is_start() const;

      phase_t     phase() const;
      void        phase(phase_t phase_);

      Freq        freq;
      Shift       shift;

   private:

      phase_t     _phase = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Freq, typename Shift>
   inline phase_t synth_base<Freq, Shift>::next()
   {
      auto prev_phase = _phase;
      _phase += freq();
      return prev_phase + shift();
   }

   template <typename Freq, typename Shift>
   inline phase_t synth_base<Freq, Shift>::get() const
   {
      return _phase + shift();
   }

   template <typename Freq, typename Shift>
   inline bool synth_base<Freq, Shift>::is_start() const
   {
      return get() < freq();
   }

   template <typename Freq, typename Shift>
   inline phase_t synth_base<Freq, Shift>::phase() const
   {
      return _phase;
   }

   template <typename Freq, typename Shift>
   inline void synth_base<Freq, Shift>::phase(phase_t phase_)
   {
      _phase = phase_;
   }
}}

#endif
