/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SLOPE_HPP_APRIL_25_2021)
#define CYCFI_Q_SLOPE_HPP_APRIL_25_2021

#include <q/support/base.hpp>
#include <q/support/literals.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/differentiator.hpp>
#include <q/fx/moving_average.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // slope tracks the slope of the signal over a period of time specified by
   // `dt` (delta time).
   ////////////////////////////////////////////////////////////////////////////
   struct slope
   {
      slope(std::size_t max_size)
       : _sum{max_size}
      {}

      slope(duration dt, std::uint32_t sps)
       : _sum{dt, sps}
      {}

      float operator()(float s)
      {
         return _sum(_diff(s));
      }

      bool operator()() const
      {
         return _sum();
      }

      differentiator          _diff;
      moving_sum              _sum;
   };
}

#endif
