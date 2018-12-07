/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SAW_HPP_DECEMBER_24_2015)
#define CYCFI_Q_SAW_HPP_DECEMBER_24_2015

#include <q/phase.hpp>
#include <q/detail/antialiasing.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // basic sawtooth-wave synthesizer (not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct basic_saw_synth
   {
      constexpr float operator()(phase p) const
      {
         constexpr float x = 2.0f / phase::one_cyc;
         return (p.val * x) - 1.0;
      }
   };

   constexpr auto basic_saw = basic_saw_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // sawtooth-wave synthesizer (bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct saw_synth
   {
      constexpr float operator()(phase p, phase dt) const
      {
         constexpr float x = 2.0f / phase::one_cyc;
         auto r = (p.val * x) - 1.0;

         // Correct discontinuity
         r -= detail::poly_blep(p, dt);
         return r;
      }
   };

   constexpr auto saw = saw_synth{};
}}

#endif
