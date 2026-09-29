/*=============================================================================
   Copyright (C) 2012-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_MESSAGES_HPP_OCTOBER_8_2012)
#define CYCFI_Q_MIDI_MESSAGES_HPP_OCTOBER_8_2012

#include <q/midi/cc.hpp>
#include <q/midi/note.hpp>
#include <cstdint>

namespace cycfi::q::midi_1_0
{
   using midi::message_base;
   namespace cc = midi::cc;

   namespace status
   {
      enum
      {
         note_off         = 0x80,
         note_on          = 0x90,
         poly_pressure    = 0xA0,
         control_change   = 0xB0,
         program_change   = 0xC0,
         channel_pressure = 0xD0,
         pitch_bend       = 0xE0,
         sysex            = 0xF0,
         song_position    = 0xF2,
         song_select      = 0xF3,
         tune_request     = 0xF6,
         sysex_end        = 0xF7,
         timing_tick      = 0xF8,
         start            = 0xFA,
         continue_        = 0xFB,
         stop             = 0xFC,
         active_sensing   = 0xFE,
         reset            = 0xFF
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // message, messageN, raw_message: Generic MIDI messages
   ////////////////////////////////////////////////////////////////////////////
   template <int size_>
   struct message : message_base
   {
      static constexpr int const size = size_;
      std::uint8_t data[size];
   };

   struct raw_message
   {
      // raw data is the 24-bit data comprising a MIDI 1.0 message.
      // The 24-bit MIDI data is encoded as little-endian:
      //
      // +--------|--------|--------|--------+
      // |  MSB   |        |        |   LSB  |
      // | unused | data-2 | data-1 | status |
      // +--------|--------|--------|--------+

      std::uint32_t data;
   };

   struct message1 : message<1>
   {
      message1() = default;
      constexpr message1(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
      }
   };

   struct message2 : message<2>
   {
      message2() = default;
      constexpr message2(raw_message msg)
      {
         data[0] = msg.data & 0xFF;
         data[1] = (msg.data >> 8) & 0xFF;
      }
   };

   struct message3 : message<3>
   {
      message3() = default;
      constexpr message3(raw_message msg)
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

      constexpr note_off(std::uint8_t channel, std::uint8_t key, std::uint8_t velocity)
      {
         data[0] = (channel & 0x0F) | status::note_off;
         data[1] = key;
         data[2] = velocity;
      }

      constexpr std::uint8_t     channel() const      { return data[0] & 0x0F; }
      constexpr std::uint8_t     key() const          { return data[1]; }
      constexpr std::uint8_t     velocity() const     { return data[2]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // note_on
   ////////////////////////////////////////////////////////////////////////////
   struct note_on : message3
   {
      using message3::message3;

      constexpr note_on(std::uint8_t channel, std::uint8_t key, std::uint8_t velocity)
      {
         data[0] = (channel & 0x0F) | status::note_on;
         data[1] = key;
         data[2] = velocity;
      }

      constexpr std::uint8_t     channel() const      { return data[0] & 0x0F; }
      constexpr std::uint8_t     key() const          { return data[1]; }
      constexpr std::uint8_t     velocity() const     { return data[2]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // poly_pressure
   ////////////////////////////////////////////////////////////////////////////
   struct poly_pressure : message3
   {
      using message3::message3;

      constexpr poly_pressure(std::uint8_t channel, std::uint8_t key, std::uint8_t pressure)
      {
         data[0] = (channel & 0x0F) | status::poly_pressure;
         data[1] = key;
         data[2] = pressure;
      }

      constexpr std::uint8_t     channel() const      { return data[0] & 0x0F; }
      constexpr std::uint8_t     key() const          { return data[1]; }
      constexpr std::uint8_t     pressure() const     { return data[2]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // control_change
   ////////////////////////////////////////////////////////////////////////////
   struct control_change : message3
   {
      using message3::message3;

      constexpr control_change(std::uint8_t channel, cc::controller ctrl, std::uint8_t value)
      {
         data[0] = (channel & 0x0F) | status::control_change;
         data[1] = ctrl;
         data[2] = value;
      }

      constexpr std::uint8_t     channel() const      { return data[0] & 0x0F; }
      constexpr cc::controller   controller() const   { return cc::controller(data[1]); }
      constexpr std::uint8_t     value() const        { return data[2]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // program_change
   ////////////////////////////////////////////////////////////////////////////
   struct program_change : message2
   {
      using message2::message2;

      constexpr program_change(std::uint8_t channel, std::uint8_t preset)
      {
         data[0] = (channel & 0x0F) | status::program_change;
         data[1] = preset;
      }

      constexpr std::uint8_t     channel() const      { return data[0] & 0x0F; }
      constexpr std::uint8_t     preset() const       { return data[1]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // channel_pressure
   ////////////////////////////////////////////////////////////////////////////
   struct channel_pressure : message2
   {
      using message2::message2;

      constexpr channel_pressure(std::uint8_t channel, std::uint8_t pressure)
      {
         data[0] = (channel & 0x0F) | status::channel_pressure;
         data[1] = pressure;
      }

      constexpr std::uint8_t     channel() const      { return data[0] & 0x0F; }
      constexpr std::uint8_t     pressure() const     { return data[1]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // pitch_bend
   ////////////////////////////////////////////////////////////////////////////
   struct pitch_bend : message3
   {
      using message3::message3;

      constexpr pitch_bend(std::uint8_t channel, std::uint16_t value)
      {
         data[0] = (channel & 0x0F) | status::pitch_bend;
         data[1] = value & 0x7F;
         data[2] = value >> 7;
      }

      constexpr pitch_bend(std::uint8_t channel, std::uint16_t lsb, std::uint8_t msb)
      {
         data[0] = (channel & 0x0F) | status::pitch_bend;
         data[1] = lsb;
         data[2] = msb;
      }

      constexpr std::uint8_t     channel() const      { return data[0] & 0x0F; }
      constexpr std::uint16_t    value() const        { return data[1] | (data[2] << 7); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // song_position
   ////////////////////////////////////////////////////////////////////////////
   struct song_position : message3
   {
      using message3::message3;

      constexpr song_position(std::uint16_t position)
      {
         data[0] = status::song_position;
         data[1] = position & 0x7F;
         data[2] = position >> 7;
      }

      constexpr song_position(std::uint8_t lsb, std::uint8_t msb)
      {
         data[0] = status::song_position;
         data[1] = lsb;
         data[2] = msb;
      }

      constexpr std::uint16_t    position() const     { return data[1] | (data[2] << 7); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // song_select
   ////////////////////////////////////////////////////////////////////////////
   struct song_select : message2
   {
      using message2::message2;

      constexpr song_select(std::uint8_t song_number)
      {
         data[0] = status::song_select;
         data[1] = song_number;
      }

      constexpr std::uint16_t    song_number() const  { return data[1]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // tune_request
   ////////////////////////////////////////////////////////////////////////////
   struct tune_request : message1
   {
      using message1::message1;

      constexpr tune_request()
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

      constexpr timing_tick()
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

      constexpr start()
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

      constexpr continue_()
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

      constexpr stop()
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

      constexpr active_sensing()
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

      constexpr reset()
      {
         data[0] = status::reset;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // sysex: a system exclusive message to send.
   //
   // The one message with no fixed length, so it carries its own markers:
   // 0xF0, a manufacturer identifier, the payload, then 0xF7. The identifier
   // is written in its three byte form, a zero followed by the two halves of
   // id, which is what a maker was given once the single byte space ran out.
   //
   // Payload bytes are masked to seven bits: the eighth marks a status byte
   // and would end the message early.
   //
   // This builds a message to send. byte_reader reads one that arrives, and
   // hands over a sysex_view.
   ////////////////////////////////////////////////////////////////////////////
   template <int size_>
   struct sysex : message<size_ + 5>
   {
      constexpr sysex(std::uint16_t id, std::uint8_t const* data_in)
      {
         this->data[0] = status::sysex;
         this->data[1] = 0;
         this->data[2] = id >> 8;
         this->data[3] = id & 0x7F;
         for (int i = 0; i != size_; ++i)
            this->data[i+4] = data_in[i] & 0x7F;
         this->data[size_ + 4] = status::sysex_end;
      }

      constexpr std::uint16_t    id() const
      {
         return (this->data[2] << 8) | this->data[3];
      }
   };

}

namespace cycfi::q::midi
{
   // The system messages keep the MIDI 1.0 byte form in both protocols:
   // MIDI 2.0 carries them unchanged.
   using midi_1_0::song_position;
   using midi_1_0::song_select;
   using midi_1_0::tune_request;
   using midi_1_0::timing_tick;
   using midi_1_0::start;
   using midi_1_0::continue_;
   using midi_1_0::stop;
   using midi_1_0::active_sensing;
   using midi_1_0::reset;
}

#endif
