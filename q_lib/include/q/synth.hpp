/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SYNTH_HPP_DECEMBER_24_2015)
#define CYCFI_Q_SYNTH_HPP_DECEMBER_24_2015

#include <q/literals.hpp>
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
    , typename std::enable_if<
         !is_arithmetic<Freq, Shift>::value &&
         !std::is_same<Freq, frequency>::value
      >::type* = 0)
   {
      return { freq, shift };
   }

   template <typename Shift, typename T, typename Int>
   inline auto sin(T freq, std::uint32_t sps, Shift shift
    , typename std::enable_if<!is_arithmetic<Shift>::value>::type* = 0)
   {
      return sin(var(phase::freq(freq, sps)), shift);
   }

   template <typename Shift>
   inline auto sin(frequency freq, std::uint32_t sps, Shift shift
    , typename std::enable_if<!is_arithmetic<Shift>::value>::type* = 0)
   {
      return sin(var(phase::freq(double(freq), sps)), shift);
   }

   template <typename T>
   inline auto sin(T freq, std::uint32_t sps, double shift)
   {
      return sin(freq, sps, var(phase::angle(shift)));
   }

   inline auto sin(frequency freq, std::uint32_t sps, double shift)
   {
      return sin(double(freq), sps, var(phase::angle(shift)));
   }

   template <typename T>
   inline auto sin(T freq, std::uint32_t sps)
   {
      return sin(freq, sps, zero_phase());
   }

   inline auto sin(frequency freq, std::uint32_t sps)
   {
      return sin(double(freq), sps, zero_phase());
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
         phase_t mod_out = detail::sin_gen(mod_synth.next()) * mgain();
         return detail::sin_gen(this->next() + mod_out);
      }

      using mod_synth_t = synth_base<mod_freq, MShift>;

      MGain          mgain;
      MFactor        mfactor;
      mod_synth_t    mod_synth;
   };

   // 16.16 bit fixed point one (1.0 representation)
   constexpr int32_t fm_fxp_one = 1 << 16;
   constexpr int32_t fm_fxp_half = fm_fxp_one / 2;

   template <typename T>
   constexpr int32_t fm_fxp(T n)
   {
      return n * fm_fxp_one;
   }

   constexpr int32_t fm_fxp(int32_t n)
   {
      return n << 16;
   }

   template <typename T>
   phase_t fm_gain(T mgain)
   {
      return fm_fxp(mgain) * fm_fxp_half;
   }

   template <typename Freq, typename Shift
    , typename MGain, typename MShift, typename MFactor>
   inline fm_synth<Freq, Shift, MGain, MShift, MFactor>
   fm(Freq freq, Shift shift, MGain mgain, MShift mshift, MFactor mfactor
    , typename std::enable_if<
         !is_arithmetic<Freq, Shift, MGain, MShift, MFactor
      >::value>::type* = 0)
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

   template <typename T>
   inline auto fm(T freq, T mgain, T mfactor, std::uint32_t sps)
   {
      return fm(
         var(phase::freq(freq, sps))
       , var(fm_gain(mgain))
       , var(mfactor)
      );
   }

   template <typename T>
   inline auto fm(T freq, T shift, T mgain, T mfactor, std::uint32_t sps)
   {
      return fm(
         var(phase::freq(freq, sps))
       , var(phase::angle(shift))
       , var(fm_gain(mgain))
       , var(mfactor)
      );
   }
}}

#endif
