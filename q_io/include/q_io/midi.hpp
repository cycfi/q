/*=============================================================================
   Copyright (C) 2012-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_HPP_OCTOBER_8_2012)
#define CYCFI_Q_MIDI_HPP_OCTOBER_8_2012

#include <cstdint>

namespace cycfi { namespace q { namespace midi
{
   namespace status
   {
      enum
      {
         note_off             = 0x80,
         note_on              = 0x90,
         poly_aftertouch      = 0xA0,
         control_change       = 0xB0,
         program_change       = 0xC0,
         channel_aftertouch   = 0xD0,
         pitch_bend           = 0xE0,
         sysex                = 0xF0,
         song_position        = 0xF2,
         song_select          = 0xF3,
         tune_request         = 0xF6,
         sysex_end            = 0xF7,
         timing_tick          = 0xF8,
         start                = 0xFA,
         continue_            = 0xFB,
         stop                 = 0xFC,
         active_sensing       = 0xFE,
         reset                = 0xFF
      };
   }

   namespace cc
   {
      enum controller
      {
         bank_select          = 0x00,
         modulation           = 0x01,
         breath               = 0x02,
         foot                 = 0x04,
         portamento_time      = 0x05,
         data_entry           = 0x06,
         channel_volume       = 0x07,
         balance              = 0x08,
         pan                  = 0x0A,
         expression           = 0x0B,
         effect_1             = 0x0C,
         effect_2             = 0x0D,
         general_1            = 0x10,
         general_2            = 0x11,
         general_3            = 0x12,
         general_4            = 0x13,

         bank_select_lsb      = 0x20,
         modulation_lsb       = 0x21,
         breath_lsb           = 0x22,
         foot_lsb             = 0x24,
         portamento_time_lsb  = 0x25,
         data_entry_lsb       = 0x26,
         channel_volume_lsb   = 0x27,
         balance_lsb          = 0x28,
         pan_lsb              = 0x2A,
         expression_lsb       = 0x2B,
         effect_1_lsb         = 0x2C,
         effect_2_lsb         = 0x2D,
         general_1_lsb        = 0x30,
         general_2_lsb        = 0x31,
         general_3_lsb        = 0x32,
         general_4_lsb        = 0x33,

         sustain              = 0x40,
         portamento           = 0x41,
         sostenuto            = 0x42,
         soft_pedal           = 0x43,
         legato               = 0x44,
         hold_2               = 0x45,

         sound_controller_1   = 0x46,  // default: sound variation
         sound_controller_2   = 0x47,  // default: timbre / harmonic content
         sound_controller_3   = 0x48,  // default: release time
         sound_controller_4   = 0x49,  // default: attack time
         sound_controller_5   = 0x4A,  // default: brightness
         sound_controller_6   = 0x4B,  // no default
         sound_controller_7   = 0x4C,  // no default
         sound_controller_8   = 0x4D,  // no default
         sound_controller_9   = 0x4E,  // no default
         sound_controller_10  = 0x4F,  // no default

         general_5            = 0x50,
         general_6            = 0x51,
         general_7            = 0x52,
         general_8            = 0x53,

         portamento_control   = 0x54,
         effects_1_depth      = 0x5B, // previously reverb send
         effects_2_depth      = 0x5C, // previously tremolo depth
         effects_3_depth      = 0x5D, // previously chorus depth
         effects_4_depth      = 0x5E, // previously celeste (detune) depth
         effects_5_depth      = 0x5F, // previously phaser effect depth
         data_inc             = 0x60, // increment data value (+1)
         data_dec             = 0x61, // decrement data value (-1)

         nonrpn_lsb           = 0x62,
         nonrpn_msb           = 0x63,
         rpn_lsb              = 0x64,
         rpn_msb              = 0x65,
         all_sounds_off       = 0x78,
         reset                = 0x79,
         local                = 0x7A,
         all_notes_off        = 0x7B,
         omni_off             = 0x7C,
         omni_on              = 0x7D,
         mono                 = 0x7E,
         poly                 = 0x7F
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // message, raw_message: Generic MIDI messages
   ////////////////////////////////////////////////////////////////////////////
   template <int size_>
   struct message
   {
      static int const size = size_;
      std::uint8_t data[size];
   };

   struct raw_message
   {
      std::uint32_t data;
   };

   ////////////////////////////////////////////////////////////////////////////
   // note_off
   ////////////////////////////////////////////////////////////////////////////
   struct note_off : message<3>
   {
      note_off(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
         data[1] = (msg.data >> 8) & 0xFF;
         data[2] = (msg.data >> 16) & 0xFF;
      }

      note_off(std::uint8_t channel, std::uint8_t key, std::uint8_t velocity)
      {
         data[0] = channel | status::note_off;
         data[1] = key;
         data[2] = velocity;
      }

      std::uint8_t   channel() const   { return data[0] & 0x0F; }
      std::uint8_t   key() const       { return data[1]; }
      std::uint8_t   velocity() const  { return data[2]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // note_on
   ////////////////////////////////////////////////////////////////////////////
   struct note_on : message<3>
   {
      note_on(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
         data[1] = (msg.data >> 8) & 0xFF;
         data[2] = (msg.data >> 16) & 0xFF;
      }

      note_on(std::uint8_t channel, std::uint8_t key, std::uint8_t velocity)
      {
         data[0] = channel | status::note_on;
         data[1] = key;
         data[2] = velocity;
      }

      std::uint8_t   channel() const   { return data[0] & 0x0F; }
      std::uint8_t   key() const       { return data[1]; }
      std::uint8_t   velocity() const  { return data[2]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // poly_aftertouch
   ////////////////////////////////////////////////////////////////////////////
   struct poly_aftertouch : message<3>
   {
      poly_aftertouch(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
         data[1] = (msg.data >> 8) & 0xFF;
         data[2] = (msg.data >> 16) & 0xFF;
      }

      poly_aftertouch(std::uint8_t channel, std::uint8_t key, std::uint8_t pressure)
      {
         data[0] = channel | status::poly_aftertouch;
         data[1] = key;
         data[2] = pressure;
      }

      std::uint8_t   channel() const   { return data[0] & 0x0F; }
      std::uint8_t   key() const       { return data[1]; }
      std::uint8_t   pressure() const  { return data[2]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // control_change
   ////////////////////////////////////////////////////////////////////////////
   struct control_change : message<3>
   {
      control_change(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
         data[1] = (msg.data >> 8) & 0xFF;
         data[2] = (msg.data >> 16) & 0xFF;
      }

      control_change(std::uint8_t channel, cc::controller ctrl, std::uint8_t value)
      {
         data[0] = channel | status::control_change;
         data[1] = ctrl;
         data[2] = value;
      }

      std::uint8_t   channel() const      { return data[0] & 0x0F; }
      cc::controller controller() const   { return cc::controller(data[1]); }
      std::uint8_t   value() const        { return data[2]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // program_change
   ////////////////////////////////////////////////////////////////////////////
   struct program_change : message<2>
   {
      program_change(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
         data[1] = (msg.data >> 8) & 0xFF;
      }

      program_change(std::uint8_t channel, std::uint8_t preset)
      {
         data[0] = channel | status::program_change;
         data[1] = preset;
      }

      std::uint8_t   channel() const   { return data[0] & 0x0F; }
      std::uint8_t   preset() const    { return data[1]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // channel_aftertouch
   ////////////////////////////////////////////////////////////////////////////
   struct channel_aftertouch : message<2>
   {
      channel_aftertouch(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
         data[1] = (msg.data >> 8) & 0xFF;
      }

      channel_aftertouch(std::uint8_t channel, std::uint8_t pressure)
      {
         data[0] = channel | status::channel_aftertouch;
         data[1] = pressure;
      }

      std::uint8_t   channel() const   { return data[0] & 0x0F; }
      std::uint8_t   pressure() const  { return data[1]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // pitch_bend
   ////////////////////////////////////////////////////////////////////////////
   struct pitch_bend : message<3>
   {
      pitch_bend(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
         data[1] = (msg.data >> 8) & 0xFF;
         data[2] = (msg.data >> 16) & 0xFF;
      }

      pitch_bend(std::uint8_t channel, std::uint16_t value)
      {
         data[0] = channel | status::pitch_bend;
         data[1] = value & 0x7F;
         data[2] = value >> 7;
      }

      pitch_bend(std::uint8_t channel, std::uint16_t lsb, std::uint8_t msb)
      {
         data[0] = channel | status::pitch_bend;
         data[1] = lsb;
         data[2] = msb;
      }

      std::uint8_t   channel() const   { return data[0] & 0x0F; }
      std::uint16_t  value() const     { return data[1] | (data[2] << 7); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // song_position
   ////////////////////////////////////////////////////////////////////////////
   struct song_position : message<3>
   {
      song_position(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
         data[1] = (msg.data >> 8) & 0xFF;
         data[2] = (msg.data >> 16) & 0xFF;
      }

      song_position(std::uint16_t position)
      {
         data[0] = status::song_position;
         data[1] = position & 0x7F;
         data[2] = position >> 7;
      }

      song_position(std::uint8_t lsb, std::uint8_t msb)
      {
         data[0] = status::song_position;
         data[1] = lsb;
         data[2] = msb;
      }

      std::uint16_t  position() const  { return data[1] | (data[2] << 7); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // song_select
   ////////////////////////////////////////////////////////////////////////////
   struct song_select : message<2>
   {
      song_select(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
         data[1] = (msg.data >> 8) & 0xFF;
      }

      song_select(std::uint8_t song_number)
      {
         data[0] = status::song_select;
         data[1] = song_number;
      }

      std::uint16_t  song_number() const  { return data[1]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // tune_request
   ////////////////////////////////////////////////////////////////////////////
   struct tune_request : message<1>
   {
      tune_request(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
      }

      tune_request()
      {
         data[0] = status::tune_request;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // timing_tick
   ////////////////////////////////////////////////////////////////////////////
   struct timing_tick : message<1>
   {
      timing_tick(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
      }

      timing_tick()
      {
         data[0] = status::timing_tick;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // start
   ////////////////////////////////////////////////////////////////////////////
   struct start : message<1>
   {
      start(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
      }

      start()
      {
         data[0] = status::start;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // continue_
   ////////////////////////////////////////////////////////////////////////////
   struct continue_ : message<1>
   {
      continue_(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
      }

      continue_()
      {
         data[0] = status::continue_;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // stop
   ////////////////////////////////////////////////////////////////////////////
   struct stop : message<1>
   {
      stop(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
      }

      stop()
      {
         data[0] = status::stop;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // active_sensing
   ////////////////////////////////////////////////////////////////////////////
   struct active_sensing : message<1>
   {
      active_sensing(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
      }

      active_sensing()
      {
         data[0] = status::active_sensing;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // reset
   ////////////////////////////////////////////////////////////////////////////
   struct reset : message<1>
   {
      reset(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
      }

      reset()
      {
         data[0] = status::reset;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // processor
   ////////////////////////////////////////////////////////////////////////////
   struct processor
   {
      void     operator()(note_off msg, std::size_t time) {}
      void     operator()(note_on msg, std::size_t time) {}
      void     operator()(poly_aftertouch msg, std::size_t time) {}
      void     operator()(control_change msg, std::size_t time) {}
      void     operator()(program_change msg, std::size_t time) {}
      void     operator()(channel_aftertouch msg, std::size_t time) {}
      void     operator()(pitch_bend msg, std::size_t time) {}
      void     operator()(song_position msg, std::size_t time) {}
      void     operator()(song_select msg, std::size_t time) {}
      void     operator()(tune_request msg, std::size_t time) {}
      void     operator()(timing_tick msg, std::size_t time) {}
      void     operator()(start msg, std::size_t time) {}
      void     operator()(continue_ msg, std::size_t time) {}
      void     operator()(stop msg, std::size_t time) {}
      void     operator()(active_sensing msg, std::size_t time) {}
      void     operator()(reset msg, std::size_t time) {}
   };
}}}

#endif
