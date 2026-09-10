/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_VA_SYNTH_2_CONTROLLER_SEPTEMBER_11_2026)
#define QPLUG_VA_SYNTH_2_CONTROLLER_SEPTEMBER_11_2026

#include <qplug/controller.hpp>

namespace qplug = cycfi::qplug;
namespace q = cycfi::q;

///////////////////////////////////////////////////////////////////////////////
// The controller declares the plugin's parameters; the base holds them.
//
// Stage 2 adds the filter: a resonant low-pass swept by an envelope of its
// own, which is where the virtual analog claim actually lives. Two
// contours, as a Minimoog or a Prophet has them: one shapes how loud the
// note is, the other how bright, and they are rarely the same shape. A
// filter that snaps open and settles under a note that swells is the
// sound the arrangement is for.
//
// Stage 1's controls keep their ids and their meaning, so a preset saved
// by stage 1 still reads here.
//
// There is no output level, since the channel fader the plugin sits on is
// already that.
///////////////////////////////////////////////////////////////////////////////
class va_synth_controller : public qplug::controller
{
public:

   using duration = q::duration;
   using decibel = q::decibel;

   enum
   {
      attack_id, decay_id, sustain_level_id, release_id
    , cutoff_id, resonance_id, env_depth_id
    , filter_attack_id, filter_decay_id, filter_sustain_level_id
    , filter_release_id
   };

   parameter_list       parameters() const override;

   duration             attack() const;
   duration             decay() const;
   decibel              sustain_level() const;
   duration             release() const;
   q::frequency         cutoff() const;
   double               resonance() const;
   double               env_depth() const;

   duration             filter_attack() const;
   duration             filter_decay() const;
   double               filter_sustain_level() const;   // 0 to 1
   duration             filter_release() const;
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

inline q::duration va_synth_controller::release() const
{
   return get_parameter<duration>(release_id);
}

inline q::frequency va_synth_controller::cutoff() const
{
   return get_parameter<q::frequency>(cutoff_id);
}

inline double va_synth_controller::resonance() const
{
   return get_parameter<double>(resonance_id);
}

inline double va_synth_controller::env_depth() const
{
   return get_parameter<double>(env_depth_id);
}

inline q::duration va_synth_controller::filter_attack() const
{
   return get_parameter<duration>(filter_attack_id);
}

inline q::duration va_synth_controller::filter_decay() const
{
   return get_parameter<duration>(filter_decay_id);
}

// How much of the depth the contour holds while a key is down, from none
// to all of it. An amount, not a loudness, so it is a plain fraction: in
// decibels most of the travel would sit between a thousandth and a tenth,
// where nothing can be heard to happen.
inline double va_synth_controller::filter_sustain_level() const
{
   return get_parameter<double>(filter_sustain_level_id) / 100.0;
}

inline q::duration va_synth_controller::filter_release() const
{
   return get_parameter<duration>(filter_release_id);
}

#endif
