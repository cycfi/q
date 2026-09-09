/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_PACKET_WRITER_HPP_SEPTEMBER_10_2026)
#define CYCFI_Q_MIDI_PACKET_WRITER_HPP_SEPTEMBER_10_2026

#include <q/midi/ump.hpp>
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
      std::span<std::uint8_t const> payload, Send&& send
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

         send(packet{
            (std::uint32_t(message_type::data64) << 28)
               | (std::uint32_t(group & 0xF) << 24)
               | (std::uint32_t(status) << 20)
               | (std::uint32_t(count) << 16)
               | (std::uint32_t(bytes[0]) << 8) | bytes[1]
          , (std::uint32_t(bytes[2]) << 24) | (std::uint32_t(bytes[3]) << 16)
               | (std::uint32_t(bytes[4]) << 8) | bytes[5]});
      }
   }
}

#endif
