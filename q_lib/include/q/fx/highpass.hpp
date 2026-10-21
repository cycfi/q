/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman, Nikos Parastatidis. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXP_HIGH_PASS_OCTOBER_21_2026)
#define CYCFI_Q_EXP_HIGH_PASS_OCTOBER_21_2026

#include <q/support/base.hpp>
#include <q/support/literals.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Basic one pole high-pass filter (6dB/Oct)
   ////////////////////////////////////////////////////////////////////////////
   struct one_pole_highpass
   {
      one_pole_highpass(float a)
       : a(a)
      {}
 
      one_pole_highpass(frequency freq, float sps)
       : a(1.0 - fast_exp(-2_pi * as_double(freq) / sps))
      {}
 
      float operator()(float s)
      {
         lp += a * (s - lp);
         return y = s - lp;
      }
 
      float operator()() const
      {
         return y;
      }
 
      one_pole_highpass& operator=(float v)
      {
         lp = v;
         y = 0.0f;
         return *this;
      }
 
      void cutoff(frequency freq, float sps)
      {
         a = 1.0 - fast_exp(-2_pi * as_double(freq) / sps);
      }
 
      float y = 0.0f, a;
      float lp = 0.0f;     // the low-pass state
   };
}

#endif
