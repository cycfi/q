/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_GAIN_CONTROLLER_SEPTEMBER_6_2026)
#define QPLUG_GAIN_CONTROLLER_SEPTEMBER_6_2026

#include <qplug/controller.hpp>

namespace qplug = cycfi::qplug;

///////////////////////////////////////////////////////////////////////////////
// The controller declares the plugin's parameters; the base holds them.
///////////////////////////////////////////////////////////////////////////////
class gain_controller : public qplug::controller
{
public:

   using decibel = cycfi::q::decibel;

   enum { volume_id };

   parameter_list       parameters() const override;

   decibel              volume() const;
   parameter const&     volume_param() const;
   model_type&          volume_model() { return model(volume_id); }
};

///////////////////////////////////////////////////////////////////////////////
// Inline implementation
///////////////////////////////////////////////////////////////////////////////
inline gain_controller::decibel gain_controller::volume() const
{
   return get_parameter<decibel>(volume_id);
}

inline gain_controller::parameter const&
gain_controller::volume_param() const
{
   return parameters()[volume_id];
}

#endif
