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
   return _volume.load();
}

// Audio thread: store, and leave the model for update_models.
void gain_controller::set_parameter(int, double value)
{
   _volume.store(value);
   _dirty.store(true);
}

// Main thread, after the host changed the value on the audio thread.
void gain_controller::update_models()
{
   if (_dirty.exchange(false))
      _volume_model = _volume.load();
}

// Main thread, from the GUI. The model is the source here, so update it
// directly; the base forwards the value to the host.
void gain_controller::edit_parameter(int id, double value)
{
   _volume_model = value;
   controller::edit_parameter(id, value);
}
