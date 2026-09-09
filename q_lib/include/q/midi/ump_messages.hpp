/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_UMP_MESSAGES_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_UMP_MESSAGES_HPP_SEPTEMBER_9_2026

#include <q/midi/ump.hpp>
#include <q/midi/messages.hpp>
#include <cstddef>
#include <cstdint>

namespace cycfi::q::midi_2_0
{
   // The same base as MIDI 1.0's messages, so one processor can take both:
   // a MIDI 2.0 stream carries MIDI 1.0 voice messages inside it.
   using midi_1_0::message_base;

   ////////////////////////////////////////////////////////////////////////////
   // packet_message: the MIDI 2.0 counterpart of MIDI 1.0's message<N>.
   //
   // Where a MIDI 1.0 message is an array of bytes, a MIDI 2.0 message is
   // an array of 32 bit words, one to four of them, and every one begins
   // with the message type and the group. A specific message derives from
   // the width it needs and reads its fields out of the words, the way
   // note_on derives from message3.
   ////////////////////////////////////////////////////////////////////////////
   template <int words_>
   struct packet_message : message_base
   {
      static constexpr int const words = words_;

      constexpr packet_message(packet const& p)
      {
         for (int i = 0; i != words; ++i)
            data[i] = p.word(i);
      }

      constexpr std::uint8_t     message_type() const
                                 { return data[0] >> 28; }
      constexpr std::uint8_t     group() const
                                 { return (data[0] >> 24) & 0xF; }
      constexpr std::uint32_t    word(std::size_t i) const
                                 { return data[i]; }

      std::uint32_t data[words];
   };

   ////////////////////////////////////////////////////////////////////////////
   // The MIDI 2.0 channel voice messages, message type 0x4. M2-104-UM,
   // section 4.2 and Table 19.
   //
   // Every one is a 64 bit packet: type and group, a four bit opcode and a
   // four bit channel, a 16 bit index whose meaning the opcode sets, and 32
   // bits of data. What changed from MIDI 1.0 is resolution, 16 bit
   // velocity and 32 bit everything else, and the messages that address a
   // single note.
   ////////////////////////////////////////////////////////////////////////////
   namespace opcode
   {
      enum
      {
         registered_per_note    = 0x0,
         assignable_per_note    = 0x1,
         registered             = 0x2,   // what RPN became
         assignable             = 0x3,   // what NRPN became
         relative_registered    = 0x4,
         relative_assignable    = 0x5,
         per_note_pitch_bend    = 0x6,
         note_off               = 0x8,
         note_on                = 0x9,
         poly_pressure          = 0xA,
         control_change         = 0xB,
         program_change         = 0xC,
         channel_pressure       = 0xD,
         pitch_bend             = 0xE,
         per_note_management    = 0xF
      };
   }

   struct voice_message : packet_message<2>
   {
      using packet_message<2>::packet_message;

      constexpr std::uint8_t     channel() const
                                 { return (data[0] >> 16) & 0xF; }
      constexpr std::uint8_t     opcode() const
                                 { return (data[0] >> 20) & 0xF; }

   protected:

      // The index field, byte 3 and byte 4 of the first word, and the
      // payload, the whole second word.
      constexpr std::uint8_t     byte3() const
                                 { return (data[0] >> 8) & 0xFF; }
      constexpr std::uint8_t     byte4() const     { return data[0] & 0xFF; }
      constexpr std::uint32_t    payload() const   { return data[1]; }
   };

   // A message about one note: the index holds the note number.
   struct note_message : voice_message
   {
      using voice_message::voice_message;

      constexpr std::uint8_t     key() const       { return byte3() & 0x7F; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 4.2.1, 4.2.2: note off and note on, with 16 bit velocity and an
   // optional attribute. A velocity of zero is a note on that is quiet, not
   // a note off.
   ////////////////////////////////////////////////////////////////////////////
   struct note_off : note_message
   {
      using note_message::note_message;

      constexpr std::uint16_t    velocity() const
                                 { return payload() >> 16; }
      constexpr std::uint8_t     attribute_type() const
                                 { return byte4(); }
      constexpr std::uint16_t    attribute() const
                                 { return payload() & 0xFFFF; }
   };

   struct note_on : note_message
   {
      using note_message::note_message;

      constexpr std::uint16_t    velocity() const
                                 { return payload() >> 16; }
      constexpr std::uint8_t     attribute_type() const
                                 { return byte4(); }
      constexpr std::uint16_t    attribute() const
                                 { return payload() & 0xFFFF; }
   };

   // 4.2.3: 32 bit pressure on one note.
   struct poly_pressure : note_message
   {
      using note_message::note_message;

      constexpr std::uint32_t    value() const     { return payload(); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 4.2.4: per-note controllers, 256 registered and 256 assignable, which
   // is what MPE spends a channel per note to approximate.
   ////////////////////////////////////////////////////////////////////////////
   struct per_note_controller : note_message
   {
      using note_message::note_message;

      constexpr std::uint8_t     index() const     { return byte4(); }
      constexpr std::uint32_t    value() const     { return payload(); }
   };

   struct registered_per_note_controller : per_note_controller
   {
      using per_note_controller::per_note_controller;
   };

   struct assignable_per_note_controller : per_note_controller
   {
      using per_note_controller::per_note_controller;
   };

   // 4.2.5: detach earlier notes on this number from per-note control, or
   // reset their controllers, so two notes of one number stay apart.
   struct per_note_management : note_message
   {
      using note_message::note_message;

      constexpr bool             detach() const    { return byte4() & 0x02; }
      constexpr bool             reset() const     { return byte4() & 0x01; }
   };

   // 4.2.12: pitch bend on one note, unsigned and centred at 0x80000000.
   struct per_note_pitch_bend : note_message
   {
      using note_message::note_message;

      constexpr std::uint32_t    value() const     { return payload(); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 4.2.6: a controller with 32 bits of value. Controllers 0, 6, 32, 38
   // and 98 to 101 are not sent in MIDI 2.0: bank select and the parameter
   // sequences became messages of their own.
   ////////////////////////////////////////////////////////////////////////////
   struct control_change : voice_message
   {
      using voice_message::voice_message;

      constexpr std::uint8_t     controller() const   { return byte3() & 0x7F; }
      constexpr std::uint32_t    value() const        { return payload(); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 4.2.7, 4.2.8: registered and assignable controllers, 128 banks of 128,
   // in one message where MIDI 1.0 needed four. The bank is what RPN or
   // NRPN called the most significant half, the index the least.
   ////////////////////////////////////////////////////////////////////////////
   struct banked_controller : voice_message
   {
      using voice_message::voice_message;

      constexpr std::uint8_t     bank() const      { return byte3() & 0x7F; }
      constexpr std::uint8_t     index() const     { return byte4() & 0x7F; }
      constexpr std::uint32_t    value() const     { return payload(); }
   };

   struct registered_controller : banked_controller
   {
      using banked_controller::banked_controller;
   };

   struct assignable_controller : banked_controller
   {
      using banked_controller::banked_controller;
   };

   // The relative forms move the value rather than set it, and cannot be
   // translated to MIDI 1.0. Their data is two's complement.
   struct relative_controller : voice_message
   {
      using voice_message::voice_message;

      constexpr std::uint8_t     bank() const      { return byte3() & 0x7F; }
      constexpr std::uint8_t     index() const     { return byte4() & 0x7F; }
      constexpr std::int32_t     value() const
                                 { return std::int32_t(payload()); }
   };

   struct relative_registered_controller : relative_controller
   {
      using relative_controller::relative_controller;
   };

   struct relative_assignable_controller : relative_controller
   {
      using relative_controller::relative_controller;
   };

   ////////////////////////////////////////////////////////////////////////////
   // 4.2.9: program and bank in one message. The bank applies only when the
   // valid bit is set; otherwise the bank fields are zero and ignored.
   ////////////////////////////////////////////////////////////////////////////
   struct program_change : voice_message
   {
      using voice_message::voice_message;

      constexpr bool             bank_valid() const
                                 { return byte4() & 0x01; }
      constexpr std::uint8_t     program() const
                                 { return (payload() >> 24) & 0x7F; }
      constexpr std::uint8_t     bank_msb() const
                                 { return (payload() >> 8) & 0x7F; }
      constexpr std::uint8_t     bank_lsb() const
                                 { return payload() & 0x7F; }
   };

   // 4.2.10, 4.2.11: 32 bit pressure and bend for the whole channel. The
   // bend is unsigned and centred at 0x80000000.
   struct channel_pressure : voice_message
   {
      using voice_message::voice_message;

      constexpr std::uint32_t    value() const     { return payload(); }
   };

   struct pitch_bend : voice_message
   {
      using voice_message::voice_message;

      static constexpr std::uint32_t centre = 0x80000000u;

      constexpr std::uint32_t    value() const     { return payload(); }
   };
}

#endif
