/*=============================================================================
   Copyright (c) 2014-2017 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SYNTH_BASE_HPP_NOVEMBER_4_2017)
#define CYCFI_Q_SYNTH_BASE_HPP_NOVEMBER_4_2017

#include <q/support.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // The synthesizers use fixed point 0.32 format computations where all
   // the bits are fractional and represents phase values that runs from
   // 0 to uint32_max (0 to 2π).
   ////////////////////////////////////////////////////////////////////////////
   using phase_t = uint32_t;
   using signed_phase_t = int32_t;

   // The turn, also cycle, full circle, revolution, and rotation, is complete
   // circular movement or measure (as to return to the same point) with circle
   // or ellipse. A turn is abbreviated τ, cyc, rev, or rot depending on the
   // application. The symbol τ can also be used as a mathematical constant to
   // represent 2π radians. (https://en.wikipedia.org/wiki/Angular_unit)

   // One complete cycle or turn:
   constexpr phase_t one_cyc = int_max<phase_t>();

   ////////////////////////////////////////////////////////////////////////////
   // osc_freq: given frequency (freq) and samples per second (sps),
   // calculate the fixed point frequency that the phase accumulator
   // (see below) requires.
   ////////////////////////////////////////////////////////////////////////////
   constexpr uint32_t osc_freq(double freq, uint32_t sps)
   {
      return (one_cyc * freq) / sps;
   }

   ////////////////////////////////////////////////////////////////////////////
   // osc_period: given period and samples per second (sps),
   // calculate the fixed point frequency that the phase accumulator
   // (see below) requires.
   ////////////////////////////////////////////////////////////////////////////
   constexpr uint32_t osc_period(double period, uint32_t sps)
   {
      return one_cyc / (sps * period);
   }

   ////////////////////////////////////////////////////////////////////////////
   // osc_period: given period in terms of number of samples,
   // calculate the fixed point frequency that the phase accumulator
   // (see below) requires. Argument samples can be fractional.
   ////////////////////////////////////////////////////////////////////////////
   constexpr uint32_t osc_period(double samples)
   {
      return one_cyc / samples;
   }

   ////////////////////////////////////////////////////////////////////////////
   // osc_phase: given phase (in radians), calculate the fixed point phase
   // that the phase accumulator (see below) requires. phase uses fixed
   // point 0.32 format and runs from 0 to uint32_max (0 to 2pi).
   ////////////////////////////////////////////////////////////////////////////
   constexpr uint32_t osc_phase(double phase)
   {
      return int_max<phase_t>() * (phase / _2pi);
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

      phase_t           next();
      phase_t           get() const;
      bool              is_start() const;

      void              period(double samples);
      void              period(double period_, uint32_t sps);
      phase_t           phase() const;
      void              phase(phase_t phase_);

      Freq              freq;
      Shift             shift;

   private:

      phase_t           _phase = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Freq, typename Shift>
   inline phase_t synth_base<Freq, Shift>::next()
   {
      auto prev_phase = _phase;
      _phase += shift() + freq();
      return prev_phase;
   }

   template <typename Freq, typename Shift>
   inline phase_t synth_base<Freq, Shift>::get() const
   {
      return _phase;
   }

   template <typename Freq, typename Shift>
   inline bool synth_base<Freq, Shift>::is_start() const
   {
      return get() < freq();
   }

   template <typename Freq, typename Shift>
   inline void synth_base<Freq, Shift>::period(double samples)
   {
      freq(osc_period(samples));
   }

   template <typename Freq, typename Shift>
   inline void synth_base<Freq, Shift>::period(double period_, uint32_t sps)
   {
      freq(osc_period(period_, sps));
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
