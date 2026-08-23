/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_SPECTRAL_FLUX_HPP_AUGUST_23_2026)
#define CYCFI_Q_SPECTRAL_FLUX_HPP_AUGUST_23_2026

#include <q/support/base.hpp>
#include <q/fx/biquad.hpp>
#include <q/fx/envelope.hpp>
#include <cmath>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // spectral_flux: the level of a signal's band above `cutoff`, peak-held
   // over `window`. A pluck or a pick scrape puts energy there that a
   // sustained string does not, so its rise against its own recent past
   // (a delta_gate on this level) reads a transient's spectrum. The block
   // is the level; the gate is the consumer's, with the consumer's bar.
   ////////////////////////////////////////////////////////////////////////////
   struct spectral_flux
   {
                     spectral_flux(
                        frequency cutoff, duration window, float sps);

      float          operator()(float s);
      float          operator()() const;

      using follower = peak_envelope_follower;

      highpass    _hp;
      follower    _env;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline spectral_flux::spectral_flux(
      frequency cutoff, duration window, float sps)
    : _hp{cutoff, sps}
    , _env{window, sps}
   {}

   inline float spectral_flux::operator()(float s)
   {
      return _env(std::abs(_hp(s)));
   }

   inline float spectral_flux::operator()() const
   {
      return _env();
   }
}

#endif
