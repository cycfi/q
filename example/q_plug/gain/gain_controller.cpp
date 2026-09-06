/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "gain_controller.hpp"

using parameter = qplug::parameter;
using parameter_list = gain_controller::parameter_list;

parameter_list gain_controller::parameters() const
{
   static parameter params[] =
   {
      parameter{ "Volume", 1.0 }.range(0.0, 2.0)
   };

   return { params };
}

double gain_controller::get_parameter(int) const
{
   return _volume;
}

void gain_controller::set_parameter(int, double value)
{
   _volume = value;
}
