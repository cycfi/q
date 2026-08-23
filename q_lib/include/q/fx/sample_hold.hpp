/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_SAMPLE_HOLD_HPP_AUGUST_23_2026)
#define CYCFI_Q_SAMPLE_HOLD_HPP_AUGUST_23_2026

#include <q/support/base.hpp>
#include <cstdint>
#include <algorithm>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // sample_hold: the value of a signal about `d` ago, without a delay line.
   // The input is sampled every d/2 and read two samples back, so the
   // lookback runs from d to 1.5d. Three floats and a counter, for the
   // cases where a signal is compared with its own recent past and a
   // sample-exact delay would be a ring buffer for nothing.
   //
   // operator()(s) advances and returns the held value as it was BEFORE
   // this sample joined: the reference is strictly the past.
   ////////////////////////////////////////////////////////////////////////////
   struct sample_hold
   {
                     sample_hold(duration d, float sps);
                     sample_hold(std::uint32_t n_samples);

      float          operator()(float s);
      float          operator()() const;

      float          _then = 0.0f;      // two holds back
      float          _last = 0.0f;      // one hold back
      float          _hold = 0.0f;      // the hold being collected
      std::uint32_t  _n;                // samples per hold
      std::uint32_t  _tick = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline sample_hold::sample_hold(duration d, float sps)
    : sample_hold(std::uint32_t(as_float(d) * sps * 0.5f))
   {}

   inline sample_hold::sample_hold(std::uint32_t n_samples)
    : _n(std::max<std::uint32_t>(1, n_samples))
   {}

   inline float sample_hold::operator()(float s)
   {
      float const then = _then;
      if (++_tick >= _n)
      {
         _tick = 0;
         _then = _last;
         _last = _hold;
         _hold = s;
      }
      return then;
   }

   inline float sample_hold::operator()() const
   {
      return _then;
   }
}

#endif
