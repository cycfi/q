/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_VA_SYNTH_CONTROLLER_SEPTEMBER_11_2026)
#define QPLUG_VA_SYNTH_CONTROLLER_SEPTEMBER_11_2026

#include <qplug/controller.hpp>

namespace qplug = cycfi::qplug;
namespace q = cycfi::q;

///////////////////////////////////////////////////////////////////////////////
// The controller declares the plugin's parameters; the base holds them.
//
// Stage 1 is the envelope and nothing else: what a note sounds like over
// time. There is no output level here, since the channel fader the plugin
// sits on is already that. Every later stage adds parameters to this list
// and changes none of these, so a preset saved by one stage still reads in
// the next.
///////////////////////////////////////////////////////////////////////////////
class va_synth_controller : public qplug::controller
{
public:

   using duration = q::duration;
   using decibel = q::decibel;

   enum
   {
      attack_id, decay_id, sustain_level_id, sustain_rate_id, release_id
   };

   parameter_list       parameters() const override;

   duration             attack() const;
   duration             decay() const;
   decibel              sustain_level() const;
   duration             sustain_rate() const;
   duration             release() const;
};

///////////////////////////////////////////////////////////////////////////////
// Inline implementation
///////////////////////////////////////////////////////////////////////////////
inline q::duration va_synth_controller::attack() const
{
   return get_parameter<duration>(attack_id);
}

inline q::duration va_synth_controller::decay() const
{
   return get_parameter<duration>(decay_id);
}

inline q::decibel va_synth_controller::sustain_level() const
{
   return get_parameter<decibel>(sustain_level_id);
}

inline q::duration va_synth_controller::sustain_rate() const
{
   return get_parameter<duration>(sustain_rate_id);
}

inline q::duration va_synth_controller::release() const
{
   return get_parameter<duration>(release_id);
}

#endif
