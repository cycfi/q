/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PERIOD_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_PERIOD_HPP_FEBRUARY_21_2018

#include <q/support/duration.hpp>
#include <q/support/frequency.hpp>

namespace cycfi::q
{
   struct frequency;

   ////////////////////////////////////////////////////////////////////////////
   struct period : duration
   {
      using duration::duration;

      constexpr explicit   period(duration d);
      constexpr explicit   period(frequency f);
   };

   ////////////////////////////////////////////////////////////////////////////
   constexpr period::period(duration d)
    : duration(d)
   {
   }

   constexpr period::period(frequency f)
    : duration(1.0 / f.rep)
   {
   }
}

#endif
