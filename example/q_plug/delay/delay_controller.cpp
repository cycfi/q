/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "delay_controller.hpp"

using parameter_list = delay_controller::parameter_list;
using namespace cycfi::q::literals;

parameter_list delay_controller::parameters() const
{
   static parameter params[] =
   {
      parameter{ 1, "Delay", 350_ms }.range(0.0, max_delay.rep),
      parameter{ 2, "Feedback", 50.0 }.range(0.0, 100.0).unit("%")
   };

   return { params };
}
