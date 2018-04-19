/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_NOTES_HPP_APRIL_20_2018)
#define CYCFI_Q_NOTES_HPP_APRIL_20_2018

#include <q/literals.hpp>

namespace cycfi { namespace q
{
   // We need this because we don't have a constexpr std::pow
   constexpr auto _12th_root = 1.059463094359295;

   constexpr frequency next_frequency(frequency f)
   {
      return f * _12th_root;
   }

   struct octave_notes
   {
      constexpr octave_notes(frequency base)
       : a     (base)
       , a_sh  (next_frequency(a))
       , b     (next_frequency(a_sh))
       , c     (next_frequency(b) / 2)
       , c_sh  (next_frequency(c))
       , d     (next_frequency(c_sh))
       , d_sh  (next_frequency(d))
       , e     (next_frequency(d_sh))
       , f     (next_frequency(e))
       , f_sh  (next_frequency(f))
       , g     (next_frequency(f_sh))
       , g_sh  (next_frequency(g))
      {}

      frequency a, a_sh, b, c, c_sh, d, d_sh, e, f, f_sh, g, g_sh;
   };

   constexpr octave_notes note[] =
   {
      { 27.5 }
    , { 55 }
    , { 110 }
    , { 220 }
    , { 440 }
    , { 880 }
    , { 1760 }
    , { 3520 }
    , { 7040 }
   };
}}

#endif

