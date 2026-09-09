/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_UMP_HPP_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_UMP_HPP_SEPTEMBER_9_2026

#include <array>
#include <cstddef>
#include <cstdint>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // The message types. Universal MIDI Packet Format and MIDI 2.0 Protocol
   // (M2-104-UM), section 2.1.4, Table 3.
   ////////////////////////////////////////////////////////////////////////////
   namespace message_type
   {
      enum
      {
         utility        = 0x0,   // 32 bits
         system         = 0x1,   // 32 bits: real time and common
         midi1_voice    = 0x2,   // 32 bits: MIDI 1.0 channel voice
         data64         = 0x3,   // 64 bits: sysex in 7 bit form
         midi2_voice    = 0x4,   // 64 bits: MIDI 2.0 channel voice
         data128        = 0x5,   // 128 bits: sysex in 8 bit form, mixed data
         flex_data      = 0xD,   // 128 bits: version 1.1, not read yet
         stream         = 0xF    // 128 bits: version 1.1, endpoint messages
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // packet_words: how many 32 bit words a packet of the given type holds.
   //
   // Every type has a size, the reserved ones included, because a receiver
   // must know how far to skip past a message it does not understand.
   ////////////////////////////////////////////////////////////////////////////
   constexpr std::size_t packet_words(std::uint8_t type)
   {
      switch (type & 0xF)
      {
         case 0x0: case 0x1: case 0x2: case 0x6: case 0x7:
            return 1;
         case 0x3: case 0x4: case 0x8: case 0x9: case 0xA:
            return 2;
         case 0xB: case 0xC:
            return 3;
         default:
            return 4;
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // packet: one Universal MIDI Packet, one to four 32 bit words.
   //
   // The first word always begins with the message type and the group,
   // section 2.1.2. What follows depends on the type: a MIDI 1.0 or 2.0
   // voice message carries a four bit status and a four bit channel, a
   // system message carries a whole status byte and no channel.
   ////////////////////////////////////////////////////////////////////////////
   struct packet
   {
      constexpr packet(std::uint32_t w0)
       : _words{w0, 0, 0, 0}
      {}

      constexpr packet(std::uint32_t w0, std::uint32_t w1)
       : _words{w0, w1, 0, 0}
      {}

      constexpr packet(std::uint32_t w0, std::uint32_t w1, std::uint32_t w2)
       : _words{w0, w1, w2, 0}
      {}

      constexpr packet(
         std::uint32_t w0, std::uint32_t w1
       , std::uint32_t w2, std::uint32_t w3)
       : _words{w0, w1, w2, w3}
      {}

      constexpr std::uint8_t  message_type() const
                              { return _words[0] >> 28; }
      constexpr std::uint8_t  group() const
                              { return (_words[0] >> 24) & 0xF; }
      constexpr std::size_t   words() const
                              { return packet_words(message_type()); }
      constexpr std::uint32_t word(std::size_t i) const
                              { return _words[i]; }

      // For the voice message types, 0x2 and 0x4.
      constexpr std::uint8_t  status() const
                              { return (_words[0] >> 20) & 0xF; }
      constexpr std::uint8_t  channel() const
                              { return (_words[0] >> 16) & 0xF; }

      // For the system type, 0x1, whose status is the whole byte.
      constexpr std::uint8_t  system_status() const
                              { return (_words[0] >> 16) & 0xFF; }

      // Whether the type is one the specification defines, version 1.1
      // included. Reserved types are sized and skipped, never read.
      constexpr bool          defined() const
                              {
                                 auto const t = message_type();
                                 return t <= message_type::data128
                                    || t == message_type::flex_data
                                    || t == message_type::stream;
                              }

   private:

      std::array<std::uint32_t, 4> _words;
   };
}

#endif
