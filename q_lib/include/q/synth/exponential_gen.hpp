/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXPONENTIAL_GEN_HPP_APRIL_26_2023)
#define CYCFI_Q_EXPONENTIAL_GEN_HPP_APRIL_26_2023

#include <q/support/base.hpp>
#include <q/support/literals.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   struct exponential_gen
   {
      static constexpr auto full = 1.0f / 0.99f;
      static constexpr auto tau = 4.6f;      // time constants to reach full

      exponential_gen(duration width, float sps)
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
}

#endif
