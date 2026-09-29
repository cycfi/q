/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_GAIN_CONTROLLER_SEPTEMBER_6_2026)
#define Q_PLUG_GAIN_CONTROLLER_SEPTEMBER_6_2026

#include <q_plug/controller.hpp>

namespace q_plug = cycfi::q_plug;

///////////////////////////////////////////////////////////////////////////////
// The controller declares the plugin's parameters; the base holds them.
///////////////////////////////////////////////////////////////////////////////
class gain_controller : public q_plug::controller
{
public:

   using decibel = cycfi::q::decibel;

   enum { volume_id };

   // The fader runs from silence, the 24 bit floor, up to +10 dB.
   static constexpr decibel silence = cycfi::q::dB(-144.0);
   static constexpr decibel max_volume = cycfi::q::dB(10.0);

   parameter_list       parameters() const override;

   decibel              volume() const;
   parameter const&     volume_param() const;
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
