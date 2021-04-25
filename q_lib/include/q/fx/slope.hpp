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
   // `dt` (delta time). The `lowest_freq` constructor parameter specifies
   // the lowest frequency of interest used to construct the
   // `fast_envelope_follower` that does the envelope tracking.
   //
   // slope returns a positive value on attacks transients within `dt` (delta
   // time) and returns negative value on decays and note releases. The
   // magnitude is indicative of the rate of attack or decay/release.
   ////////////////////////////////////////////////////////////////////////////
   struct slope
   {
      slope(duration dt, frequency lowest_freq, std::uint32_t sps)
       : _peak{lowest_freq.period()*0.6, sps}
       , _sum{dt, sps}
      {}

      float operator()(float s)
      {
         return _sum(_diff(_peak(s*s)));
      }

      bool operator()() const
      {
         return _sum();
      }

      fast_envelope_follower  _peak;
      differentiator          _diff;
      moving_sum              _sum;
   };
}

#endif
