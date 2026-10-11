/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PERIOD_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_PERIOD_HPP_FEBRUARY_21_2018

#include <q/support/duration.hpp>

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
}

// frequency::period() returns a complete period, so frequency is included
// only once period is declared.
#include <q/support/frequency.hpp>

namespace cycfi::q
{
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
