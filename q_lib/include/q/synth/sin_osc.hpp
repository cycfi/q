/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SIN_OSC_HPP_DECEMBER_24_2015)
#define CYCFI_Q_SIN_OSC_HPP_DECEMBER_24_2015

#include <q/support/phase.hpp>
#include <q/detail/sin_table.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // sin_osc: Sine wave Oscillator.
   ////////////////////////////////////////////////////////////////////////////
   struct sin_osc
   {
      constexpr float operator()(phase p) const
      {
         return sin_lu(p);
      }

      constexpr float operator()(phase_iterator i) const
      {
         return (*this)(i._phase);
      }
   };

   constexpr auto sin = sin_osc{};
}

#endif
