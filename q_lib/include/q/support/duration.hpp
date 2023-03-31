/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DURATION_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_DURATION_HPP_FEBRUARY_21_2018

#include <cstdint>
#include <cmath>
#include <q/support/base.hpp>
#include <q/support/value.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   struct duration : value<double, duration>
   {
      using base_type = value<double, duration>;
      using base_type::base_type;

      constexpr explicit   duration(double val) : base_type(val) {}
   };

   // Free functions
   constexpr double  as_double(duration d);
   constexpr float   as_float(duration d);

   ////////////////////////////////////////////////////////////////////////////
   constexpr double as_double(duration d)
   {
      return d.rep;
   }

   constexpr float as_float(duration d)
   {
      return d.rep;
   }
}

#endif
