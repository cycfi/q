/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman, Nikos Parastatidis. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXP_SMOOTHING_OCTOBER_22_2026)
#define CYCFI_Q_EXP_SMOOTHING_OCTOBER_22_2026

#include <cmath>
#include <limits>
#include <q/support/base.hpp>
#include <q/support/literals.hpp>
#include <q/fx/lowpass.hpp>

namespace cycfi::q
{
   ///////////////////////////////////////////////////////////////////////////////
   // Ensures float-safe snap
   ///////////////////////////////////////////////////////////////////////////////
   struct snap
   {
      explicit snap(float delta) : _delta(delta) {}

      float operator()(float s, float target)
      {
         _settled = std::abs(s - target) <= _delta || s == _prev;
         _prev = s;
         return _settled ? target : s;
      }

      bool settled() const { return _settled; }

      float _delta;
      float _prev = std::numeric_limits<float>::quiet_NaN();
      bool _settled = false;
   };

   ///////////////////////////////////////////////////////////////////////////////
   // Smoothed value parameter to be used with any compatible smoother
   // eg.
   // q::smoothed<q::one_pole_lowpass>  gain{10_ms, sps, 1e-4f};
   // q::smoothed<q::dynamic_smoother>  cutoff{10_ms, sps, 0.1f};
   //
   // float g = gain(target);   // per sample
   ///////////////////////////////////////////////////////////////////////////////
   template <typename Smoother>
   struct smoothed
   {
      smoothed(duration time, float sps, float delta)
         : _smoother(1.0f / (2_pi * as_float(smoothing_time)), sps), _snap(delta)
      {
      }

      float operator()(float target)
      {
         return _snap(_smoother(_target), _target);
      }

      float operator()() const { return _smoother(); }

      smoothed &operator=(float v) { _smoother = v; }

      bool settled() const { return _snap.settled(); }

      Smoother _smoother;
      snap _snap;
   };
}

#endif