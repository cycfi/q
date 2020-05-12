/*=============================================================================
   Copyright (C) 2012-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_HPP_OCTOBER_8_2012)
#define CYCFI_Q_MIDI_HPP_OCTOBER_8_2012

#include <cstdint>
#include <q/support/notes.hpp>

#if defined(B0)
# undef B0
#endif

namespace cycfi::q::midi
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
         effects_1_depth      = 0x5B,  // previously reverb send
         effects_2_depth      = 0x5C,  // previously tremolo depth
         effects_3_depth      = 0x5D,  // previously chorus depth
         effects_4_depth      = 0x5E,  // previously celeste (detune) depth
         effects_5_depth      = 0x5F,  // previously phaser effect depth
         data_inc             = 0x60,  // increment data value (+1)
         data_dec             = 0x61,  // decrement data value (-1)

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
   // message, messageN, raw_message: Generic MIDI messages
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

   struct message1 : message<1>
   {
      message1() = default;
      message1(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
      }
   };

   struct message2 : message<2>
   {
      message2() = default;
      message2(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
         data[1] = (msg.data >> 8) & 0xFF;
      }
   };

   struct message3 : message<3>
   {
      message3() = default;
      message3(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
         data[1] = (msg.data >> 8) & 0xFF;
         data[2] = (msg.data >> 16) & 0xFF;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // note_off
   ////////////////////////////////////////////////////////////////////////////
   struct note_off : message3
   {
      using message3::message3;

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
   struct note_on : message3
   {
      using message3::message3;

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
   struct poly_aftertouch : message3
   {
      using message3::message3;

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
   struct control_change : message3
   {
      using message3::message3;

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
   struct program_change : message2
   {
      using message2::message2;

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
   struct channel_aftertouch : message2
   {
      using message2::message2;

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
   struct pitch_bend : message3
   {
      using message3::message3;

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
   struct song_position : message3
   {
      using message3::message3;

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
   struct song_select : message2
   {
      using message2::message2;

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
   struct tune_request : message1
   {
      using message1::message1;

      tune_request()
      {
         data[0] = status::tune_request;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // timing_tick
   ////////////////////////////////////////////////////////////////////////////
   struct timing_tick : message1
   {
      using message1::message1;

      timing_tick()
      {
         data[0] = status::timing_tick;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // start
   ////////////////////////////////////////////////////////////////////////////
   struct start : message1
   {
      using message1::message1;

      start()
      {
         data[0] = status::start;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // continue_
   ////////////////////////////////////////////////////////////////////////////
   struct continue_ : message1
   {
      using message1::message1;

      continue_()
      {
         data[0] = status::continue_;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // stop
   ////////////////////////////////////////////////////////////////////////////
   struct stop : message1
   {
      using message1::message1;

      stop()
      {
         data[0] = status::stop;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // active_sensing
   ////////////////////////////////////////////////////////////////////////////
   struct active_sensing : message1
   {
      using message1::message1;

      active_sensing()
      {
         data[0] = status::active_sensing;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // reset
   ////////////////////////////////////////////////////////////////////////////
   struct reset : message1
   {
      using message1::message1;

      reset()
      {
         data[0] = status::reset;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // MIDI note to frequency
   ////////////////////////////////////////////////////////////////////////////
   constexpr frequency note_frequency(std::uint8_t key)
   {
      constexpr std::uint8_t lowest_key = 9;
      constexpr std::uint8_t highest_key = 119;

      if (key < lowest_key || key > highest_key)
         return frequency(0);
      auto octave = (key - lowest_key) / 12;
      auto semitone = (key - lowest_key) % 12;
      return note_frequencies[octave][semitone];
   }

   ////////////////////////////////////////////////////////////////////////////
   // MIDI note name
   ////////////////////////////////////////////////////////////////////////////
   constexpr char const* note_name(std::uint8_t key)
   {
      constexpr char const* name[] =
      {
         "C-1", "C#-1", "D-1", "D#-1", "E-1", "F-1", "F#-1", "G-1", "G#-1", "A-1", "A#-1", "B-1",
         "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0", "A0", "A#0", "B0",
         "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",
         "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
         "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
         "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
         "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5",
         "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6",
         "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7",
         "C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8", "A8", "A#8", "B8",
         "C9", "C#9", "D9", "D#9", "E9", "F9", "F#9", "G9"
      };

      return (key < 128)? name[key] : "--";
   }

   ////////////////////////////////////////////////////////////////////////////
   // MIDI note
   ////////////////////////////////////////////////////////////////////////////
   enum class note : std::uint8_t
   {
      C0 = 12
    , Cs0, Db0 = Cs0
    , D0
    , Ds0, Eb0 = Ds0
    , E0
    , F0
    , Fs0, Gb0 = Fs0
    , G0
    , Gs0, Ab0 = Gs0

    , A0 = 21
    , As0, Bb0 = As0
    , B0
    , C1
    , Cs1, Db1 = Cs1
    , D1
    , Ds1, Eb1 = Ds1
    , E1
    , F1
    , Fs1, Gb1 = Fs1
    , G1
    , Gs1, Ab1 = Gs1

    , A1 = 33
    , As1, Bb1 = As1
    , B1
    , C2
    , Cs2, Db2 = Cs2
    , D2
    , Ds2, Eb2 = Ds2
    , E2
    , F2
    , Fs2, Gb2 = Fs2
    , G2
    , Gs2, Ab2 = Gs2

    , A2 = 45
    , As2, Bb2 = As2
    , B2
    , C3
    , Cs3, Db3 = Cs3
    , D3
    , Ds3, Eb3 = Ds3
    , E3
    , F3
    , Fs3, Gb3 = Fs3
    , G3
    , Gs3, Ab3 = Gs3

    , A3 = 57
    , As3, Bb3 = As3
    , B3
    , C4
    , Cs4, Db4 = Cs4
    , D4
    , Ds4, Eb4 = Ds4
    , E4
    , F4
    , Fs4, Gb4 = Fs4
    , G4
    , Gs4, Ab4 = Gs4

    , A4 = 69
    , As4, Bb4 = As4
    , B4
    , C5
    , Cs5, Db5 = Cs5
    , D5
    , Ds5, Eb5 = Ds5
    , E5
    , F5
    , Fs5, Gb5 = Fs5
    , G5
    , Gs5, Ab5 = Gs5

    , A5 = 81
    , As5, Bb5 = As5
    , B5
    , C6
    , Cs6, Db6 = Cs6
    , D6
    , Ds6, Eb6 = Ds6
    , E6
    , F6
    , Fs6, Gb6 = Fs6
    , G6
    , Gs6, Ab6 = Gs6

    , A6 = 93
    , As6, Bb6 = As6
    , B6
    , C7
    , Cs7, Db7 = Cs7
    , D7
    , Ds7, Eb7 = Ds7
    , E7
    , F7
    , Fs7, Gb7 = Fs7
    , G7
    , Gs7, Ab7 = Gs7

    , A7 = 105
    , As7, Bb7 = As7
    , B7
    , C8
    , Cs8, Db8 = Cs8
    , D8
    , Ds8, Eb8 = Ds8
    , E8
    , F8
    , Fs8, Gb8 = Fs8
    , G8
    , Gs8, Ab8 = Gs8

    , A8 = 117
    , As8, Bb8 = As8
    , B8
    , C9
    , Cs9, Db9 = Cs9
    , D9
    , Ds9, Eb9 = Ds9
    , E9
    , F9
    , Fs9, Gb9 = Fs9
    , G9
   };

   ////////////////////////////////////////////////////////////////////////////
   // Key (e.g. "C5") to MIDI note number
   //
   // Returns the MIDI note number given the key (string).
   // Returns -1 if parsing failed.
   ////////////////////////////////////////////////////////////////////////////
   inline int note_number(std::string_view note)
   {
      std::uint8_t n;
      auto iter = note.begin();
      if (iter != note.end())
      {
         // Get the base frequency
         switch (std::toupper(*iter++))
         {
            case 'A':   n = int(note::A0); break;
            case 'B':   n = int(note::B0); break;
            case 'C':   n = int(note::C0); break;
            case 'D':   n = int(note::D0); break;
            case 'E':   n = int(note::E0); break;
            case 'F':   n = int(note::F0); break;
            case 'G':   n = int(note::G0); break;
         }

         if (iter != note.end())
         {
            // See if we want to shift up or down by a
            // semitone.
            if (*iter == '#')
            {
               ++n;
               ++iter;
            }
            else if (*iter == 'b')
            {
               --n;
               ++iter;
            }

            if (iter != note.end())
            {
               if (std::isdigit(*iter))
               {
                  // Set the octave
                  int oct = *iter++ - '0';
                  if (iter == note.end())
                     return n + oct * 12;
               }
            }
         }
      }
      return -1;
   }

   ////////////////////////////////////////////////////////////////////////////
   // processor
   ////////////////////////////////////////////////////////////////////////////
   struct processor
   {
      void     operator()(note_on msg, std::size_t time) {}
      void     operator()(note_off msg, std::size_t time) {}
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

   template <typename Processor>
   inline void dispatch(raw_message msg, std::size_t time, Processor&& proc)
   {
      switch (msg.data & 0xFF) // status
      {
         case status::note_off:
            proc(note_off{ msg }, time);
            break;

         case status::note_on:
            proc(note_on{ msg }, time);
            break;

         case status::poly_aftertouch:
            proc(poly_aftertouch{ msg }, time);
            break;

         case status::control_change:
            proc(control_change{ msg }, time);
            break;

         case status::program_change:
            proc(program_change{ msg }, time);
            break;

         case status::channel_aftertouch:
            proc(channel_aftertouch{ msg }, time);
            break;

         case status::pitch_bend:
            proc(pitch_bend{ msg }, time);
            break;

         case status::song_position:
            proc(song_position{ msg }, time);
            break;

         case status::song_select:
            proc(song_select{ msg }, time);
            break;

         case status::tune_request:
            proc(tune_request{ msg }, time);
            break;

         case status::timing_tick:
            proc(timing_tick{ msg }, time);
            break;

         case status::start:
            proc(start{ msg }, time);
            break;

         case status::continue_:
            proc(continue_{ msg }, time);
            break;

         case status::stop:
            proc(stop{ msg }, time);
            break;

         case status::active_sensing:
            proc(active_sensing{ msg }, time);
            break;

         case status::reset:
            proc(reset{ msg }, time);
            break;
      }
   }
}

#endif
