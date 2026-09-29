/*=============================================================================
   Copyright (C) 2012-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_CC_HPP_SEPTEMBER_18_2026)
#define CYCFI_Q_MIDI_CC_HPP_SEPTEMBER_18_2026

#include <cstdint>
#include <q/support/base.hpp>

namespace cycfi::q::midi
{
   ////////////////////////////////////////////////////////////////////////////
   // What the two protocols share. MIDI 2.0 widened the values, not the
   // meanings: a controller keeps its number and a key its note, and every
   // message of either protocol derives from the one message_base, which
   // is what lets a single processor take both. midi_1_0 and midi_2_0 each
   // name what is here, so code written against either finds it there.
   ////////////////////////////////////////////////////////////////////////////
   struct message_base {}; // Base class for all messages

   namespace cc
   {
      enum controller
      {
         bank_select           = 0x00,
         modulation            = 0x01,
         breath                = 0x02,
         foot                  = 0x04,
         portamento_time       = 0x05,
         data_entry            = 0x06,
         channel_volume        = 0x07,
         balance               = 0x08,
         pan                   = 0x0A,
         expression            = 0x0B,
         effect_1              = 0x0C,
         effect_2              = 0x0D,
         general_1             = 0x10,
         general_2             = 0x11,
         general_3             = 0x12,
         general_4             = 0x13,

         bank_select_lsb       = 0x20,
         modulation_lsb        = 0x21,
         breath_lsb            = 0x22,
         foot_lsb              = 0x24,
         portamento_time_lsb   = 0x25,
         data_entry_lsb        = 0x26,
         channel_volume_lsb    = 0x27,
         balance_lsb           = 0x28,
         pan_lsb               = 0x2A,
         expression_lsb        = 0x2B,
         effect_1_lsb          = 0x2C,
         effect_2_lsb          = 0x2D,
         general_1_lsb         = 0x30,
         general_2_lsb         = 0x31,
         general_3_lsb         = 0x32,
         general_4_lsb         = 0x33,

         sustain               = 0x40,
         portamento            = 0x41,
         sostenuto             = 0x42,
         soft_pedal            = 0x43,
         legato                = 0x44,
         hold_2                = 0x45,

         sound_controller_1    = 0x46,  // default: sound variation
         sound_controller_2    = 0x47,  // default: timbre / harmonic content
         sound_controller_3    = 0x48,  // default: release time
         sound_controller_4    = 0x49,  // default: attack time
         sound_controller_5    = 0x4A,  // default: brightness
         sound_controller_6    = 0x4B,  // no default
         sound_controller_7    = 0x4C,  // no default
         sound_controller_8    = 0x4D,  // no default
         sound_controller_9    = 0x4E,  // no default
         sound_controller_10   = 0x4F,  // no default

         general_5             = 0x50,
         general_6             = 0x51,
         general_7             = 0x52,
         general_8             = 0x53,

         portamento_control    = 0x54,
         effects_1_depth       = 0x5B,  // previously reverb send
         effects_2_depth       = 0x5C,  // previously tremolo depth
         effects_3_depth       = 0x5D,  // previously chorus depth
         effects_4_depth       = 0x5E,  // previously celeste (detune) depth
         effects_5_depth       = 0x5F,  // previously phaser effect depth
         data_inc              = 0x60,  // increment data value (+1)
         data_dec              = 0x61,  // decrement data value (-1)

         nrpn_lsb              = 0x62,
         nrpn_msb              = 0x63,
         rpn_lsb               = 0x64,
         rpn_msb               = 0x65,
         all_sound_off         = 0x78,
         reset_all_controllers = 0x79,
         local_control         = 0x7A,
         all_notes_off         = 0x7B,
         omni_off              = 0x7C,
         omni_on               = 0x7D,
         mono_mode             = 0x7E,
         poly_mode             = 0x7F
      };
   }
}

#endif
