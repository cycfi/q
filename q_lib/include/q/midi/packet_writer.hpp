/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_PACKET_WRITER_HPP_SEPTEMBER_10_2026)
#define CYCFI_Q_MIDI_PACKET_WRITER_HPP_SEPTEMBER_10_2026

#include <q/midi/ump.hpp>
#include <q/midi/processor.hpp>
#include <cstddef>
#include <cstdint>
#include <span>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // send_sysex7: a system exclusive payload as the packets that carry it.
   // M2-104-UM section 4.4, the counterpart of packet_reader's gathering.
   //
   //    send_sysex7(payload, send);      // send(packet const&)
   //
   // The payload is what lies between 0xF0 and 0xF7; the brackets are not
   // sent, since the packet form has none. Six bytes go in each packet.
   // One that holds the whole payload is marked complete; otherwise the
   // first is a start, the last an end, and any between are continues.
   // Unused bytes are zero, as 4.4 requires.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Send>
   inline void send_sysex7(
      byte_span payload, Send&& send
    , std::uint8_t group = 0)
   {
      constexpr std::size_t per_packet = 6;
      auto const size = payload.size();
      auto const packets = size == 0? 1 : (size + per_packet - 1) / per_packet;

      for (std::size_t i = 0; i != packets; ++i)
      {
         std::uint8_t status = 0x0;               // complete
         if (packets > 1)
            status = i == 0? 0x1 : (i+1 == packets? 0x3 : 0x2);

         auto const offset = i * per_packet;
         auto const count = std::size_t(
            size - offset < per_packet? size - offset : per_packet);

         std::uint8_t bytes[per_packet] = {};
         for (std::size_t j = 0; j != count; ++j)
            bytes[j] = payload[offset + j] & 0x7F;

         midi::detail::emit(send, packet{
            (std::uint32_t(message_type::data64) << 28)
               | (std::uint32_t(group & 0xF) << 24)
               | (std::uint32_t(status) << 20)
               | (std::uint32_t(count) << 16)
               | (std::uint32_t(bytes[0]) << 8) | bytes[1]
          , (std::uint32_t(bytes[2]) << 24) | (std::uint32_t(bytes[3]) << 16)
               | (std::uint32_t(bytes[4]) << 8) | bytes[5]});
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // send_sysex8: a system exclusive payload in its 8 bit form, as the
   // packets that carry it. M2-104-UM section 4.5 and Table 20, the
   // counterpart of packet_reader's gathering.
   //
   //    send_sysex8(payload, stream, send);      // send(packet const&)
   //
   // A payload byte keeps all eight of its bits, where the 7 bit form
   // allows only seven, so the message cannot reach a MIDI 1.0 device.
   // Thirteen bytes go in each packet, behind the stream id that tells
   // interleaved messages apart.
   // One packet holding the whole payload is marked complete; otherwise
   // the first is a start, the last an end, and any between are continues.
   //
   // The count a packet declares includes the stream id, so it runs from
   // 1 to 14 rather than 0 to 13. An empty payload still sends one packet,
   // counting the stream id and nothing else. Unused bytes are zero, as
   // 4.5 requires.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Send>
   inline void send_sysex8(
      byte_span payload, std::uint8_t stream
    , Send&& send, std::uint8_t group = 0)
   {
      constexpr std::size_t per_packet = 13;
      auto const size = payload.size();
      auto const packets = size == 0? 1 : (size + per_packet - 1) / per_packet;

      for (std::size_t i = 0; i != packets; ++i)
      {
         std::uint8_t status = 0x0;               // complete
         if (packets > 1)
            status = i == 0? 0x1 : (i+1 == packets? 0x3 : 0x2);

         auto const offset = i * per_packet;
         auto const count = std::size_t(
            size - offset < per_packet? size - offset : per_packet);

         std::uint8_t b[per_packet] = {};
         for (std::size_t j = 0; j != count; ++j)
            b[j] = payload[offset + j];

         midi::detail::emit(send, packet{
            (std::uint32_t(message_type::data128) << 28)
               | (std::uint32_t(group & 0xF) << 24)
               | (std::uint32_t(status) << 20)
               | (std::uint32_t(count + 1) << 16)
               | (std::uint32_t(stream) << 8) | b[0]
          , (std::uint32_t(b[1]) << 24) | (std::uint32_t(b[2]) << 16)
               | (std::uint32_t(b[3]) << 8) | b[4]
          , (std::uint32_t(b[5]) << 24) | (std::uint32_t(b[6]) << 16)
               | (std::uint32_t(b[7]) << 8) | b[8]
          , (std::uint32_t(b[9]) << 24) | (std::uint32_t(b[10]) << 16)
               | (std::uint32_t(b[11]) << 8) | b[12]});
      }
   }
}

#endif
