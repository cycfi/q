/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_UMP_UTILITY_HPP_SEPTEMBER_18_2026)
#define CYCFI_Q_MIDI_UMP_UTILITY_HPP_SEPTEMBER_18_2026

#include <q/midi/ump_messages.hpp>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // The utility messages, message type 0x0. M2-104-UM version 1.1.2,
   // section 7.2.
   //
   // These are about the stream rather than about music: a packet that
   // means nothing, the two that carry a sender's clock, and the two that
   // time the events of a clip file. They may sit between the packets of a
   // message that spans several and change nothing, which is why
   // packet_reader lets them through.
   //
   // Where every other message has a group, a utility message has four
   // reserved bits, so none of these is addressed to a group. Version 1.0
   // of the specification had a group here and 1.1 took it away.
   ////////////////////////////////////////////////////////////////////////////
   namespace utility_status
   {
      enum
      {
         noop                    = 0x0,
         jr_clock                = 0x1,
         jr_timestamp            = 0x2,
         ticks_per_quarter_note  = 0x3,
         delta_clockstamp        = 0x4
      };
   }

   struct utility_message : packet_message<1>
   {
      using packet_message<1>::packet_message;

      constexpr std::uint8_t     status() const
                                 { return (data[0] >> 20) & 0xF; }

      // group() is inherited and reads the reserved bits. It means nothing
      // here and is zero in anything Q builds.

   protected:

      constexpr utility_message(std::uint8_t status, std::uint32_t value)
       : packet_message<1>{packet{
            (std::uint32_t(message_type::utility) << 28)
               | (std::uint32_t(status & 0xF) << 20)
               | (value & 0xFFFFF)}}
      {}
   };

   // 7.2.1: a packet that does nothing, and is not an error.
   struct noop : utility_message
   {
      using utility_message::utility_message;

      constexpr noop()
       : utility_message{utility_status::noop, 0}
      {}
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.2.2: the jitter reduction pair, which differ only in what the count
   // means. A clock says what time it is at the sender; a timestamp says
   // when the packet after it should be rendered.
   //
   // Both count ticks of 1/31250 of a second, which is 32 microseconds, in
   // sixteen bits, so they wrap about every 2.09712 seconds. 7.2.2.1 asks
   // a sender for a clock at least every 250 milliseconds, which is what
   // lets a receiver tell one wrap from the next.
   ////////////////////////////////////////////////////////////////////////////
   struct jr_message : utility_message
   {
      using utility_message::utility_message;

      static constexpr double    seconds_per_tick = 1.0 / 31250.0;

      constexpr std::uint16_t    ticks() const
                                 { return data[0] & 0xFFFF; }
   };

   struct jr_clock : jr_message
   {
      using jr_message::jr_message;

      constexpr explicit jr_clock(std::uint16_t ticks)
       : jr_message{utility_status::jr_clock, ticks}
      {}
   };

   struct jr_timestamp : jr_message
   {
      using jr_message::jr_message;

      constexpr explicit jr_timestamp(std::uint16_t ticks)
       : jr_message{utility_status::jr_timestamp, ticks}
      {}
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.2.3: how a clip file says when things happen. One message sets the
   // unit, in ticks to the quarter note, and the other counts ticks since
   // the event before. A tick is a fraction of a quarter note, so what it
   // is worth in seconds follows the tempo.
   //
   // 7.2.3.2: "The Delta Clockstamp message declares the time of all
   // following messages which occur before the next Delta Clockstamp
   // message", so events sharing a time share one of these. Outside a clip
   // file most receivers ignore both.
   ////////////////////////////////////////////////////////////////////////////
   struct ticks_per_quarter_note : utility_message
   {
      using utility_message::utility_message;

      // 7.2.3.1: 1 to 65535, and zero is reserved.
      constexpr explicit ticks_per_quarter_note(std::uint16_t ticks)
       : utility_message{utility_status::ticks_per_quarter_note, ticks}
      {}

      constexpr std::uint16_t    ticks() const
                                 { return data[0] & 0xFFFF; }
   };

   struct delta_clockstamp : utility_message
   {
      using utility_message::utility_message;

      // Twenty bits, and it does not wrap: 7.2.3.2 says a file with
      // nothing in it for this long gets a clockstamp and a no-op to
      // start the count again.
      static constexpr std::uint32_t max_ticks = 0xFFFFF;

      constexpr explicit delta_clockstamp(std::uint32_t ticks)
       : utility_message{utility_status::delta_clockstamp, ticks}
      {}

      constexpr std::uint32_t    ticks() const
                                 { return data[0] & 0xFFFFF; }
   };
}

#endif
