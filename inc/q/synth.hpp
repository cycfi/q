/*=============================================================================
   Copyright (c) 2014-2017 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SYNTH_HPP_DECEMBER_24_2015)
#define CYCFI_Q_SYNTH_HPP_DECEMBER_24_2015

#include <q/synth_base.hpp>
#include <q/fx.hpp>
#include <q/detail/sin_table.hpp>
#include <type_traits>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // zero_phase: Returns a simple lambda function that returns a zero phase.
   //             Use this if you don't care about synth phase shifts.
   ////////////////////////////////////////////////////////////////////////////
   auto zero_phase()
   {
      return []{ return phase_t{0}; };
   }

   ////////////////////////////////////////////////////////////////////////////
   // Sin synthesizer: Synthesizes sine waves.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Freq, typename Shift>
   struct sin_synth : synth_base<Freq, Shift>
   {
      using base_t = synth_base<Freq, Shift>;
      using base_t::base_t;

      float operator()()
      {
         return detail::sin_gen(this->next());
      }
   };

   template <typename Freq, typename Shift>
   inline sin_synth<Freq, Shift>
   sin(Freq freq, Shift shift
    , typename std::enable_if<!is_arithmetic<Freq, Shift>::value>::type* = 0)
   {
      return { freq, shift };
   }

   template <typename Shift>
   inline auto sin(double freq, uint32_t sps, Shift shift
    , typename std::enable_if<!is_arithmetic<Shift>::value>::type* = 0)
   {
      return sin(var(osc_freq(freq, sps)), shift);
   }

   inline auto sin(double freq, uint32_t sps, double shift)
   {
      return sin(freq, sps, var(osc_phase(shift)));
   }

   inline auto sin(double freq, uint32_t sps)
   {
      return sin(freq, sps, zero_phase());
   }

   ////////////////////////////////////////////////////////////////////////////
   // FM synthesizer
   ////////////////////////////////////////////////////////////////////////////
   template <
      typename Freq, typename Shift
    , typename MGain, typename MShift, typename MFactor>
   struct fm_synth : synth_base<Freq, Shift>
   {
      using base_t = synth_base<Freq, Shift>;

      fm_synth(
         Freq freq, Shift shift
       , MGain mgain, MShift mshift, MFactor mfactor
      )
       : base_t(freq, shift)
       , mgain(mgain)
       , mfactor(mfactor)
       , mod_synth(mod_freq{*this}, mshift)
      {}

      struct mod_freq
      {
         mod_freq(fm_synth& fm)
          : fm(fm) {}

         phase_t operator()() const
         {
            return fm.freq() * fm.mfactor();
         };

         fm_synth& fm;
      };

      float operator()()
      {
         signed_phase_t mod_out = detail::sin_gen(mod_synth.next()) * mgain();
         return detail::sin_gen(this->next() + mod_out);
      }

      using mod_synth_t = synth_base<mod_freq, MShift>;

      MGain          mgain;
      MFactor        mfactor;
      mod_synth_t    mod_synth;
   };

   template <typename Freq, typename Shift
    , typename MGain, typename MShift, typename MFactor>
   inline fm_synth<Freq, Shift, MGain, MShift, MFactor>
   fm(Freq freq, Shift shift, MGain mgain, MShift mshift, MFactor mfactor)
   {
      return { freq, shift, mgain, mshift, mfactor };
   }

   template <typename Freq, typename Shift, typename MGain, typename MFactor>
   inline auto
   fm(Freq freq, Shift shift, MGain mgain, MFactor mfactor
    , typename std::enable_if<
         !is_arithmetic<Freq, Shift, MGain, MFactor>::value>::type* = 0)
   {
      return fm(freq, shift, mgain, zero_phase(), mfactor);
   }

   template <typename Freq, typename MGain, typename MFactor>
   inline auto
   fm(Freq freq, MGain mgain, MFactor mfactor)
   {
      return fm(freq, zero_phase(), mgain, zero_phase(), mfactor);
   }

   ////////////////////////////////////////////////////////////////////////////
   // fixed point utilities
   ////////////////////////////////////////////////////////////////////////////

   // 16.16 bit fixed point one (1.0 representation)
   constexpr int32_t fm_fxp_one = 65536;

   constexpr int32_t fm_fxp(double n)
   {
      return n * fm_fxp_one;
   }

   constexpr int32_t fm_fxp(int32_t n)
   {
      return n << 16;
   }

   phase_t fm_gain(double mgain)
   {
      return fm_fxp(mgain) * 32767;
   }

   inline auto fm(double freq, double mgain, float mfactor, uint32_t sps)
   {
      return fm(
         var(osc_freq(freq, sps))
       , var(fm_gain(mgain))
       , var(mfactor)
      );
   }

/*
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

   // ////////////////////////////////////////////////////////////////////////////
   // // sin wave synthesizer
   // ////////////////////////////////////////////////////////////////////////////
   // class sin : public basic_synth
   // {
   // public:

   //    sin(uint32_t freq)
   //     : basic_synth(freq)
   //    {}

   //    sin(float freq, uint32_t sps)
   //     : basic_synth(freq, sps)
   //    {}

   //    float operator()()
   //    {
   //       return detail::sin_gen(base());
   //    }

   //    float operator()(int32_t offset)
   //    {
   //       return detail::sin_gen(base(offset));
   //    }
   // };

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
      phase_t mgain; // modulator gain
   };
*/

}}

#endif
