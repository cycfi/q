/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXPONENTIAL_GEN_HPP_APRIL_28_2023)
#define CYCFI_Q_EXPONENTIAL_GEN_HPP_APRIL_28_2023

#include <q/support/base.hpp>
#include <q/support/literals.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Exponential growth generator.
   //
   // An exponential growth generator generates an exponentially increasing
   // amplitude from 0.0 to 1.0, similar to a capacitor charged by a series
   // resistor over time specified by the duration (width) and samples per
   // second (sps) parameters.
   //
   // The `cv` constructor and config parameter determines the curvature of
   // the exponential. Valid `cv` values range greater than 0.0 to anything
   // less than 1.0. `cv` is the final value in the exponential curve that
   // will be considered as the peak value that will be scaled up to 1.0.
   // Increasing the `cv` value leads to more pronounced curves. Lower `cv`
   // values produce flatter, more linear ramps. The default is 0.95.
   ////////////////////////////////////////////////////////////////////////////
   struct exponential_growth_gen
   {
      exponential_growth_gen(duration width, float sps, float cv = 0.95)
       : _tau{-std::log(1.0f - cv)}
       , _full{1.0f / cv}
       , _rate{std::exp(-_tau / (sps * as_double(width)))}
      {
      }

      float operator()()
      {
         _y = _full + _rate * (_y - _full);
         return _y;
      }

      void config(duration width, float sps)
      {
         _rate = std::exp(-_tau / (sps * as_double(width)));
      }

      void config(duration width, float sps, float cv)
      {
         _tau = -std::log(1.0f - cv);
         _full = 1.0f / cv;
         _rate = std::exp(-_tau / (sps * as_double(width)));
      }

      void reset()
      {
         _y = 0;
      }

   private:

      float    _tau;
      float    _full;
      double   _rate;
      double   _y = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Exponential decay generator.
   //
   // The inverse of the exponential growth generator. This is similar to a
   // capacitor discherged through a resistor.
   ////////////////////////////////////////////////////////////////////////////
   struct exponential_decay_gen : exponential_growth_gen
   {
      exponential_decay_gen(duration width, float sps)
       : exponential_growth_gen{width, sps}
      {
      }

      float operator()()
      {
         return 1.0f - exponential_growth_gen::operator()();
      }
   };
}

#endif
