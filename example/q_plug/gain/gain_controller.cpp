/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "gain_controller.hpp"

using parameter_list = gain_controller::parameter_list;
using namespace cycfi::q::literals;

parameter_list gain_controller::parameters() const
{
   static parameter params[] =
   {
      // The bottom is the 24 bit floor: silence in anything a converter
      // can carry.
      parameter{ 1, "Volume", 0_dB }.range(-144.0, 10.0)
   };

   return { params };
}
