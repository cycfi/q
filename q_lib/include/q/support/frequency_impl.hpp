/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FREQUENCY_IMPL_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_FREQUENCY_IMPL_HPP_FEBRUARY_21_2018

#include <q/support/frequency.hpp>
#include <q/support/period.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   constexpr q::period frequency::period() const
   {
      return q::period{1.0 / rep};
   }

   constexpr double as_double(frequency f)
   {
      return f.rep;
   }

   constexpr float as_float(frequency f)
   {
      return f.rep;
   }
}

#endif
