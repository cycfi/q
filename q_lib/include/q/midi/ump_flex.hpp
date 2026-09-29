/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_UMP_FLEX_HPP_SEPTEMBER_18_2026)
#define CYCFI_Q_MIDI_UMP_FLEX_HPP_SEPTEMBER_18_2026

#include <q/midi/ump_messages.hpp>
#include <string_view>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // Flex data, message type 0xD. M2-104-UM version 1.1.2, section 7.5.
   //
   // What a score knows and a note does not: the tempo, the time and key
   // signatures, what the metronome should do, the name of the chord, and
   // the words being sung. A sequencer sends these; a synth may ignore all
   // of them.
   //
   // Every one is 128 bits, and the first word says which it is: a two bit
   // form for messages that span packets, a two bit address saying whether
   // it is meant for a channel or the whole group, the channel, and then a
   // status bank and a status. Bank 0 is the setup and performance events
   // below; banks 1 and 2 are text, gathered by packet_reader.
   ////////////////////////////////////////////////////////////////////////////
   namespace flex_format
   {
      enum
      {
         complete = 0x0,
         start    = 0x1,
         continue_= 0x2,
         end      = 0x3
      };
   }

   // 7.5.1, Table 10.
   namespace flex_address
   {
      enum
      {
         channel = 0x0,       // the channel field says which
         group   = 0x1        // the whole group, and the channel is zero
      };
   }

   // 7.5.1, Table 11.
   namespace flex_bank
   {
      enum
      {
         setup     = 0x00,    // tempo, signatures, metronome, chord name
         metadata  = 0x01,    // names, a copyright, a date, a place
         performance = 0x02   // lyrics and ruby, and their languages
      };
   }

   // The statuses of bank 0.
   namespace flex_status
   {
      enum
      {
         set_tempo          = 0x00,
         set_time_signature = 0x01,
         set_metronome      = 0x02,
         set_key_signature  = 0x05,
         set_chord_name     = 0x06
      };
   }

   struct flex_message : packet_message<4>
   {
      using packet_message<4>::packet_message;

      constexpr std::uint8_t     form() const
                                 { return (data[0] >> 22) & 0x3; }
      constexpr std::uint8_t     address() const
                                 { return (data[0] >> 20) & 0x3; }
      constexpr std::uint8_t     channel() const
                                 { return (data[0] >> 16) & 0xF; }
      constexpr std::uint8_t     status_bank() const
                                 { return (data[0] >> 8) & 0xFF; }
      constexpr std::uint8_t     status() const     { return data[0] & 0xFF; }

   protected:

      constexpr flex_message(
         std::uint8_t group, std::uint8_t form, std::uint8_t address
       , std::uint8_t channel, std::uint8_t bank, std::uint8_t status
       , std::uint32_t w1 = 0, std::uint32_t w2 = 0, std::uint32_t w3 = 0)
       : packet_message<4>{packet{
            (std::uint32_t(message_type::flex_data) << 28)
               | (std::uint32_t(group & 0xF) << 24)
               | (std::uint32_t(form & 0x3) << 22)
               | (std::uint32_t(address & 0x3) << 20)
               | (std::uint32_t(channel & 0xF) << 16)
               | (std::uint32_t(bank) << 8) | status
          , w1, w2, w3}}
      {}
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.5.3: the tempo, as how long a quarter note lasts rather than how
   // many fit in a minute. The unit is ten nanoseconds, so a quarter note
   // of half a second is 50,000,000. It is sent to the group, complete in
   // one packet.
   ////////////////////////////////////////////////////////////////////////////
   struct set_tempo : flex_message
   {
      using flex_message::flex_message;

      constexpr explicit set_tempo(
         std::uint32_t ten_nanoseconds, std::uint8_t group = 0)
       : flex_message{
            group, flex_format::complete, flex_address::group, 0
          , flex_bank::setup, flex_status::set_tempo, ten_nanoseconds}
      {}

      constexpr std::uint32_t    ten_nanoseconds() const  { return data[1]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.5.4: the time signature. The denominator is the power of two, so 2
   // is a quarter note and 3 an eighth, and zero means something the
   // specification has no way to say. The last field is Standard MIDI
   // File's count of 1/32 notes in 24 MIDI clocks.
   ////////////////////////////////////////////////////////////////////////////
   struct set_time_signature : flex_message
   {
      using flex_message::flex_message;

      constexpr set_time_signature(
         std::uint8_t numerator, std::uint8_t denominator
       , std::uint8_t thirty_seconds, std::uint8_t group = 0)
       : flex_message{
            group, flex_format::complete, flex_address::group, 0
          , flex_bank::setup, flex_status::set_time_signature
          , (std::uint32_t(numerator) << 24)
               | (std::uint32_t(denominator) << 16)
               | (std::uint32_t(thirty_seconds) << 8)}
      {}

      constexpr std::uint8_t     numerator() const
                                 { return (data[1] >> 24) & 0xFF; }
      constexpr std::uint8_t     denominator() const
                                 { return (data[1] >> 16) & 0xFF; }
      constexpr std::uint8_t     thirty_seconds() const
                                 { return (data[1] >> 8) & 0xFF; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.5.5: what the metronome does. Clicks are counted in MIDI clocks, 24
   // to the quarter note, so 24 is a click every quarter note and 36 every
   // dotted quarter. The three accents divide a bar, and 7.5.5 says they
   // add up to the beats in it; two subdivision counts allow two at once.
   ////////////////////////////////////////////////////////////////////////////
   struct set_metronome : flex_message
   {
      using flex_message::flex_message;

      constexpr set_metronome(
         std::uint8_t clocks_per_click, std::uint8_t accent_1
       , std::uint8_t accent_2 = 0, std::uint8_t accent_3 = 0
       , std::uint8_t subdivision_1 = 0, std::uint8_t subdivision_2 = 0
       , std::uint8_t group = 0)
       : flex_message{
            group, flex_format::complete, flex_address::group, 0
          , flex_bank::setup, flex_status::set_metronome
          , (std::uint32_t(clocks_per_click) << 24)
               | (std::uint32_t(accent_1) << 16)
               | (std::uint32_t(accent_2) << 8) | accent_3
          , (std::uint32_t(subdivision_1) << 24)
               | (std::uint32_t(subdivision_2) << 16)}
      {}

      constexpr std::uint8_t     clocks_per_click() const
                                 { return (data[1] >> 24) & 0xFF; }
      constexpr std::uint8_t     accent_1() const
                                 { return (data[1] >> 16) & 0xFF; }
      constexpr std::uint8_t     accent_2() const
                                 { return (data[1] >> 8) & 0xFF; }
      constexpr std::uint8_t     accent_3() const  { return data[1] & 0xFF; }
      constexpr std::uint8_t     subdivision_1() const
                                 { return (data[2] >> 24) & 0xFF; }
      constexpr std::uint8_t     subdivision_2() const
                                 { return (data[2] >> 16) & 0xFF; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.5.7, 7.5.8: the key and the chord. A note is 1 for A through 7 for
   // G, and 0 means unknown. Sharps and flats are a signed four bit count,
   // positive for sharps, and -8 means unknown or not standard. Unlike the
   // three above, these may be sent to a channel rather than a group.
   ////////////////////////////////////////////////////////////////////////////
   namespace tonic
   {
      enum
      {
         unknown = 0x0, a = 0x1, b = 0x2, c = 0x3
       , d = 0x4, e = 0x5, f = 0x6, g = 0x7
      };
   }

   struct set_key_signature : flex_message
   {
      using flex_message::flex_message;

      // Not standard, and the value the specification gives it.
      static constexpr std::int8_t unknown_key = -8;

      constexpr set_key_signature(
         std::int8_t sharps_or_flats, std::uint8_t note
       , std::uint8_t address = flex_address::group
       , std::uint8_t channel = 0, std::uint8_t group = 0)
       : flex_message{
            group, flex_format::complete, address, channel
          , flex_bank::setup, flex_status::set_key_signature
          , (std::uint32_t(sharps_or_flats & 0xF) << 28)
               | (std::uint32_t(note & 0xF) << 24)}
      {}

      // Two's complement in four bits: sharps are positive, flats negative.
      constexpr std::int8_t      sharps_or_flats() const
      {
         auto const v = std::uint8_t((data[1] >> 28) & 0xF);
         return std::int8_t(v & 0x8? int(v) - 16 : int(v));
      }

      constexpr std::uint8_t     note() const
                                 { return (data[1] >> 24) & 0xF; }
   };

   // 7.5.8, Table 14. The ones a chord symbol names most often; the
   // specification defines up to 0x1B.
   namespace chord_type
   {
      enum
      {
         clear = 0x00, major = 0x01, major_6th = 0x02, major_7th = 0x03
       , major_9th = 0x04, major_11th = 0x05, major_13th = 0x06
       , minor = 0x07, minor_6th = 0x08, minor_7th = 0x09
       , minor_9th = 0x0A, minor_11th = 0x0B, minor_13th = 0x0C
       , dominant = 0x0D, dominant_9th = 0x0E, dominant_11th = 0x0F
       , dominant_13th = 0x10, augmented = 0x11, augmented_7th = 0x12
       , diminished = 0x13, diminished_7th = 0x14, half_diminished = 0x15
       , major_minor = 0x16, pedal = 0x17, power = 0x18
       , suspended_2nd = 0x19, suspended_4th = 0x1A, seven_suspended_4th = 0x1B
      };
   }

   // 7.5.8: what an alteration does to a degree of the chord.
   namespace alteration_type
   {
      enum
      {
         none = 0, add = 1, subtract = 2, raise = 3, lower = 4
      };
   }

   struct chord_alteration
   {
      std::uint8_t   type;
      std::uint8_t   degree;
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.5.8: the chord, as a tonic, a type, up to four alterations, and a
   // bass that may be a chord of its own with two more. A bass note of 0
   // with its sharps and flats set to -8 means the bass is the tonic.
   ////////////////////////////////////////////////////////////////////////////
   struct set_chord_name : flex_message
   {
      using flex_message::flex_message;

      static constexpr std::int8_t same_as_tonic = -8;

      constexpr std::int8_t      tonic_sharps_or_flats() const
                                 { return nibble_signed(data[1] >> 28); }
      constexpr std::uint8_t     tonic_note() const
                                 { return (data[1] >> 24) & 0xF; }
      constexpr std::uint8_t     type() const
                                 { return (data[1] >> 16) & 0xFF; }

      // Four on the chord, in the order the specification lays them out.
      constexpr chord_alteration alteration(std::size_t i) const
      {
         // The type is the higher nibble of the pair, the degree the lower.
         auto const w = i < 2? data[1] : data[2];
         auto const shift = i < 2? (12 - i*8) : (28 - (i-2)*8);
         return {
            std::uint8_t((w >> shift) & 0xF)
          , std::uint8_t((w >> (shift-4)) & 0xF)};
      }

      constexpr std::int8_t      bass_sharps_or_flats() const
                                 { return nibble_signed(data[3] >> 28); }
      constexpr std::uint8_t     bass_note() const
                                 { return (data[3] >> 24) & 0xF; }
      constexpr std::uint8_t     bass_type() const
                                 { return (data[3] >> 16) & 0xFF; }

      // And two on the bass.
      constexpr chord_alteration bass_alteration(std::size_t i) const
      {
         auto const shift = 12 - i*8;
         return {
            std::uint8_t((data[3] >> shift) & 0xF)
          , std::uint8_t((data[3] >> (shift-4)) & 0xF)};
      }

   private:

      static constexpr std::int8_t nibble_signed(std::uint32_t v)
      {
         auto const n = std::uint8_t(v & 0xF);
         return std::int8_t(n & 0x8? int(n) - 16 : int(n));
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.5.9: the text messages of banks 1 and 2, gathered by packet_reader
   // the way it gathers the stream text messages. The text is UTF-8 with
   // no byte order mark, twelve bytes to a packet and at most 384 in all,
   // and refers to the reader's buffer for the duration of the call.
   //
   // Which text it is takes both the bank and the status: bank 1 status 2
   // is the name of the piece, bank 2 status 1 is a syllable of the words.
   ////////////////////////////////////////////////////////////////////////////
   namespace metadata_status
   {
      enum
      {
         unknown = 0x00, project = 0x01, composition = 0x02, clip = 0x03
       , copyright = 0x04, composer = 0x05, lyricist = 0x06, arranger = 0x07
       , publisher = 0x08, performer = 0x09, accompanist = 0x0A
       , date = 0x0B, location = 0x0C
      };
   }

   namespace performance_status
   {
      enum
      {
         unknown = 0x00, lyric = 0x01, lyric_language = 0x02
       , ruby = 0x03, ruby_language = 0x04
      };
   }

   struct flex_text_view : message_base
   {
      constexpr flex_text_view(
         std::uint8_t group, std::uint8_t address, std::uint8_t channel
       , std::uint8_t bank, std::uint8_t status, std::string_view text)
       : _group(group), _address(address), _channel(channel)
       , _bank(bank), _status(status), _text(text)
      {}

      constexpr std::uint8_t     group() const     { return _group; }
      constexpr std::uint8_t     address() const   { return _address; }
      constexpr std::uint8_t     channel() const   { return _channel; }
      constexpr std::uint8_t     status_bank() const { return _bank; }
      constexpr std::uint8_t     status() const    { return _status; }
      constexpr std::string_view text() const      { return _text; }

   private:

      std::uint8_t      _group;
      std::uint8_t      _address;
      std::uint8_t      _channel;
      std::uint8_t      _bank;
      std::uint8_t      _status;
      std::string_view  _text;
   };
}

#endif
