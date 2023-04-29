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
   // We use a time constant of 4.6 to have the signal reach approximately
   // 99% of the full signal. We set the full signal to 1/0.99 or 1.0101, in
   // order to bring the rising signal closer to the desired 1.0 value.
   ////////////////////////////////////////////////////////////////////////////
   struct exponential_growth_gen
   {
      static constexpr auto full = 1.0f / 0.99f;
      static constexpr auto tau = 4.6f; // time constants to reach full

       exponential_growth_gen(duration width, float sps)
       : _rate{fast_exp3(-tau / (sps * as_double(width)))}
      {
      }

      float operator()()
      {
         _y = full + _rate * (_y - full);
         return _y;
      }

      void config(duration width, float sps)
      {
         _rate = fast_exp3(-tau / (sps * as_double(width)));
      }

      void reset()
      {
         _y = 0;
      }

   private:

      float _rate;
      float _y = 0;
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
