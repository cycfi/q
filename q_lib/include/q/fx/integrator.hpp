/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_INTEGRATOR_HPP_DECEMBER_24_2015)
#define CYCFI_Q_INTEGRATOR_HPP_DECEMBER_24_2015

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // The integrator accumulates the input samples (s).
   ////////////////////////////////////////////////////////////////////////////
   struct integrator
   {
      integrator(float gain = 0.1)
       : _gain(gain) {}

      float operator()(float s)
      {
         return (y += _gain * s);
      }

      integrator& operator=(float y_)
      {
         y = y_;
         return *this;
      }

      float y = 0.0f;   // current output value
      float _gain;      // gain
   };
}

#endif
