/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "gain_controller.hpp"

using parameter_list = gain_controller::parameter_list;
using namespace cycfi::q::literals;

parameter_list gain_controller::parameters() const
{
   static parameter params[] =
   {
      parameter{ 1, "Volume", 0_dB }.range(silence.rep, max_volume.rep)
   };

   return { params };
}
