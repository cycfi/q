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
// time. Four controls, the classic ones: the sustain holds its level until
// the key comes up. There is no output level here, since the channel
// fader the plugin sits on is already that. Every later stage adds
// parameters to this list and changes none of these, so a preset saved by
// one stage still reads in the next.
///////////////////////////////////////////////////////////////////////////////
class va_synth_controller : public qplug::controller
{
public:

   using duration = q::duration;
   using decibel = q::decibel;

   enum
   {
      attack_id, decay_id, sustain_level_id, release_id, velocity_id
    , volume_id
   };

   parameter_list       parameters() const override;

   duration             attack() const;
   duration             decay() const;
   double               sustain_level() const;   // 0 to 1
   duration             release() const;
   double               velocity() const;         // 0 to 1

   decibel              volume() const;
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

// A fraction of full level, as a Minimoog's sustain is: half way up is
// -6 dB. In decibels, three quarters of the travel would sit below
// -15 dB, where a held note is all but gone, and the range a note lives
// in would be squeezed into the top.
inline double va_synth_controller::sustain_level() const
{
   return get_parameter<double>(sustain_level_id) / 100.0;
}

inline q::duration va_synth_controller::release() const
{
   return get_parameter<duration>(release_id);
}

// How much of the key's velocity reaches the loudness: all of it, none
// of it, or a mix. At none every note is the same, which some sounds
// want.
inline double va_synth_controller::velocity() const
{
   return get_parameter<double>(velocity_id) / 100.0;
}

// The instrument's own level. Sixteen voices at once are a great deal
// louder than one, and the clipper below is a last resort, not a mixer:
// this is what the player backs off with, and it is in decibels because
// a fader is.
inline q::decibel va_synth_controller::volume() const
{
   return get_parameter<decibel>(volume_id);
}

#endif
