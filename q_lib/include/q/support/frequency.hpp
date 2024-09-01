/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FREQUENCY_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_FREQUENCY_HPP_FEBRUARY_21_2018

#include <q/support/duration.hpp>

namespace cycfi::q
{
   struct period;

   ////////////////////////////////////////////////////////////////////////////
   struct frequency_unit;

   struct frequency : unit<double, frequency>
   {
      using base_type = unit<double, frequency>;
      using base_type::base_type;
      using unit_type = frequency_unit;

      constexpr q::period  period() const;
   };

   // Free functions
   constexpr double  as_double(frequency f);
   constexpr float   as_float(frequency f);
}

#endif
#include <q/support/frequency_impl.hpp>

