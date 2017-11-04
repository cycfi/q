/*=============================================================================
   Copyright (c) 2014-2017 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SYNTH_HPP_DECEMBER_24_2015)
#define CYCFI_Q_SYNTH_HPP_DECEMBER_24_2015

#include <q/support.hpp>
#include <q/fx.hpp>
#include <q/fixed_point.hpp>
#include <q/detail/sin_table.hpp>
#include <cstdint>

namespace cycfi { namespace q
{
   // The synthesizers use unsigned fixed point 0.32 format computations
   // where all the bits are fractional and represents phase values that
   // runs from 0 to uint32_max (0 to 2pi)

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
   // that the phase accumulator (see below) requires. phase runs from
   // 0 to uint32_max (0 to 2pi)
   ////////////////////////////////////////////////////////////////////////////
   constexpr phase_t osc_phase(double phase)
   {
      return phase_t{1.0} * (phase / _2pi);
   }

   ////////////////////////////////////////////////////////////////////////////
   // accum phase synthesizer
   ////////////////////////////////////////////////////////////////////////////
   class accum
   {
   public:

      accum(phase_t freq)
       : _freq(freq)
       , _phase(0.0)
      {}

      accum(float freq, uint32_t sps)
       : _freq(osc_freq(freq, sps))
       , _phase(0.0)
      {}

      // synthesize at offset 0
      uint32_t operator()()
      {
         auto val = _phase;
         _phase += _freq;
         return val.rep();
      }

      // synthesize at given offset --delays or advances the
      // position by offset number of samples.
      uint32_t operator()(int32_t offset)
      {
         auto val = _phase;
         _phase += _freq;
         return (val + (offset * _freq)).rep();
      }

      bool is_phase_start() const
      {
         // return true if we are at the start phase
         return _phase < _freq;
      }

      phase_t freq() const                      { return _freq; }
      phase_t phase() const                     { return _phase; }

      void freq(phase_t freq)                   { _freq = freq; }
      void freq(double freq, uint32_t sps)      { _freq = osc_freq(freq, sps); }

      void period(double samples)               { _freq = osc_period(samples); }
      void period(double period_, uint32_t sps) { _freq = osc_period(period_, sps); }

      void phase(phase_t phase)                 { _phase = phase; }

      void incr()                               { _phase += _freq; }
      void decr()                               { _phase -= _freq; }

   private:

      phase_t _freq;
      phase_t _phase;
   };

   ////////////////////////////////////////////////////////////////////////////
   // basic synthesizer
   ////////////////////////////////////////////////////////////////////////////
   class basic_synth
   {
   public:

      basic_synth(phase_t freq)
       : base(freq)
      {}

      basic_synth(float freq, uint32_t sps)
       : base(freq, sps)
      {}

      bool is_phase_start() const               { return base.is_phase_start(); }
      phase_t freq() const                      { return base.freq(); }
      phase_t phase() const                     { return base.phase(); }

      void freq(phase_t freq)                   { base.freq(freq); }
      void freq(uint32_t freq, uint32_t sps)    { base.freq(freq, sps); }

      void period(double samples)               { base.period(samples); }
      void period(double period_, uint32_t sps) { base.period(period_, sps); }

      void phase(phase_t phase)                 { base.phase(phase); }

      void incr()                               { base.incr(); }
      void decr()                               { base.decr(); }

   protected:

      accum base;
   };

   ////////////////////////////////////////////////////////////////////////////
   // pulse synthesizer (this is not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   class pulse : public basic_synth
   {
   public:

      pulse(uint32_t freq, uint32_t width)
       : basic_synth(freq)
       , width(width)
      {}

      pulse(float freq, float width, uint32_t sps)
       : basic_synth(freq, sps)
       , width(width)
      {}

      float operator()()
      {
         return base() > width.rep() ? 1.0f : -1.0f;
      }

      float operator()(int32_t offset)
      {
         return base(offset) > width.rep() ? 1.0f : -1.0f;
      }

   private:

      phase_t width;
   };

   ////////////////////////////////////////////////////////////////////////////
   // sin wave synthesizer
   ////////////////////////////////////////////////////////////////////////////
   class sin : public basic_synth
   {
   public:

      sin(uint32_t freq)
       : basic_synth(freq)
      {}

      sin(float freq, uint32_t sps)
       : basic_synth(freq, sps)
      {}

      float operator()()
      {
         return detail::sin_gen(base());
      }

      float operator()(int32_t offset)
      {
         return detail::sin_gen(base(offset));
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // two operator FM synth
   ////////////////////////////////////////////////////////////////////////////
   class fm : public basic_synth
   {
   public:

      fm(uint32_t mfreq, float mgain_, uint32_t cfreq)
       : basic_synth(cfreq)
       , mbase(mfreq)
       , mgain(mgain_)
      {
      }

      fm(float mfreq, float mgain_, float cfreq, uint32_t sps)
       : basic_synth(cfreq, sps)
       , mbase(mfreq, sps)
       , mgain(mgain_)
      {
      }

      float operator()()
      {
         auto modulator_out = detail::sin_gen(mbase()) * mgain;
         return detail::sin_gen(base() + modulator_out.rep());
      }

      float operator()(int32_t offset)
      {
         auto modulator_out = detail::sin_gen(mbase(offset)) * mgain;
         return detail::sin_gen(base(offset) + modulator_out.rep());
      }

      float modulator_gain() const        { return float(mgain); }
      void  modulator_gain(float mgain_)  { mgain = double(mgain_); }
      accum& modulator()                  { return mbase; }
      accum const& modulator() const      { return mbase; }

   private:

      using fp_32 = fixed_point<int32_t, 32>;

      accum mbase;   // modulator phase synth
      fp_32 mgain;   // modulator gain (0.32 bit signed fixed point)
   };
}}

#endif
