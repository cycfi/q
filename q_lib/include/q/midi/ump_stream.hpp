/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_UMP_STREAM_HPP_SEPTEMBER_10_2026)
#define CYCFI_Q_MIDI_UMP_STREAM_HPP_SEPTEMBER_10_2026

#include <q/midi/ump_messages.hpp>
#include <cstdint>
#include <string_view>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // UMP Stream Messages, message type 0xF. M2-104-UM version 1.1, section
   // 7.1 and Table 33.
   //
   // These are addressed to the endpoint rather than to a group or channel,
   // and they are how two endpoints find out about each other: what the
   // other end is, which protocol it speaks, and what function blocks it
   // has. A host sends endpoint discovery first, before anything else.
   //
   // Every one is 128 bits. The first word carries the type, a two bit form
   // saying whether the message is complete in this packet or spans
   // several, and a ten bit status naming the message.
   ////////////////////////////////////////////////////////////////////////////
   namespace stream_status
   {
      enum
      {
         endpoint_discovery              = 0x00,
         endpoint_info                   = 0x01,
         device_identity                 = 0x02,
         endpoint_name                   = 0x03,
         product_instance_id             = 0x04,
         stream_configuration_request    = 0x05,
         stream_configuration            = 0x06,
         function_block_discovery        = 0x10,
         function_block_info             = 0x11,
         function_block_name             = 0x12,
         start_of_clip                   = 0x20,
         end_of_clip                     = 0x21
      };
   }

   struct stream_message : packet_message<4>
   {
      using packet_message<4>::packet_message;

      // 7.1: complete in one packet, or the start, a continuation, or the
      // end of one that spans several.
      constexpr std::uint8_t     form() const
                                 { return (data[0] >> 26) & 0x3; }
      constexpr std::uint16_t    status() const
                                 { return (data[0] >> 16) & 0x3FF; }

   protected:

      constexpr std::uint8_t     byte2() const
                                 { return (data[0] >> 8) & 0xFF; }
      constexpr std::uint8_t     byte3() const     { return data[0] & 0xFF; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.1.1: what a host sends first. The filter says which of five replies
   // it wants, and each bit set gets one.
   ////////////////////////////////////////////////////////////////////////////
   struct endpoint_discovery : stream_message
   {
      using stream_message::stream_message;

      constexpr std::uint8_t     version_major() const   { return byte2(); }
      constexpr std::uint8_t     version_minor() const   { return byte3(); }
      constexpr std::uint8_t     filter() const
                                 { return data[1] & 0x1F; }

      constexpr bool             wants_info() const
                                 { return filter() & 0x01; }
      constexpr bool             wants_device_identity() const
                                 { return filter() & 0x02; }
      constexpr bool             wants_name() const
                                 { return filter() & 0x04; }
      constexpr bool             wants_product_instance_id() const
                                 { return filter() & 0x08; }
      constexpr bool             wants_stream_configuration() const
                                 { return filter() & 0x10; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.1.2: the reply that describes the endpoint. Table 33 word 2 holds a
   // static flag and the block count in the top byte, the two protocol
   // capabilities in the second lowest, and the two timestamp capabilities
   // in the lowest.
   ////////////////////////////////////////////////////////////////////////////
   struct endpoint_info : stream_message
   {
      using stream_message::stream_message;

      constexpr std::uint8_t     version_major() const   { return byte2(); }
      constexpr std::uint8_t     version_minor() const   { return byte3(); }
      constexpr bool             static_function_blocks() const
                                 { return data[1] & 0x80000000u; }
      constexpr std::uint8_t     function_blocks() const
                                 { return (data[1] >> 24) & 0x7F; }
      constexpr bool             midi2() const   { return data[1] & 0x1000; }
      constexpr bool             midi1() const   { return data[1] & 0x0100; }
      constexpr bool             receives_jr() const
                                 { return data[1] & 0x2; }
      constexpr bool             transmits_jr() const
                                 { return data[1] & 0x1; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.1.3: the same four fields the MIDI 1.0 device inquiry carries, with
   // family and model least significant byte first, seven bits each.
   ////////////////////////////////////////////////////////////////////////////
   struct device_identity : stream_message
   {
      using stream_message::stream_message;

      constexpr std::uint32_t    manufacturer() const
                                 { return data[1] & 0xFFFFFF; }
      constexpr std::uint16_t    family() const
                                 {
                                    return ((data[2] >> 24) & 0x7F)
                                       | (((data[2] >> 16) & 0x7F) << 7);
                                 }
      constexpr std::uint16_t    model() const
                                 {
                                    return ((data[2] >> 8) & 0x7F)
                                       | ((data[2] & 0x7F) << 7);
                                 }
      constexpr std::uint32_t    revision() const  { return data[3]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.1.6: which protocol a stream carries and whether timestamps ride
   // with it. The request asks, the notification states.
   ////////////////////////////////////////////////////////////////////////////
   namespace protocol
   {
      enum
      {
         midi1 = 0x01,
         midi2 = 0x02
      };
   }

   struct stream_configuration_message : stream_message
   {
      using stream_message::stream_message;

      constexpr std::uint8_t     protocol() const        { return byte2(); }
      constexpr bool             receives_jr() const
                                 { return byte3() & 0x2; }
      constexpr bool             transmits_jr() const
                                 { return byte3() & 0x1; }
   };

   struct stream_configuration_request : stream_configuration_message
   {
      using stream_configuration_message::stream_configuration_message;
   };

   struct stream_configuration : stream_configuration_message
   {
      using stream_configuration_message::stream_configuration_message;
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.1.7 to 7.1.9: function blocks, the parts of an endpoint. A block
   // number of 0xFF asks about all of them.
   ////////////////////////////////////////////////////////////////////////////
   struct function_block_discovery : stream_message
   {
      using stream_message::stream_message;

      static constexpr std::uint8_t all = 0xFF;

      constexpr std::uint8_t     block() const     { return byte2(); }
      constexpr std::uint8_t     filter() const    { return byte3() & 0x3; }
      constexpr bool             wants_info() const{ return filter() & 0x1; }
      constexpr bool             wants_name() const{ return filter() & 0x2; }
   };

   struct function_block_info : stream_message
   {
      using stream_message::stream_message;

      constexpr bool             active() const    { return data[0] & 0x8000; }
      constexpr std::uint8_t     block() const     { return byte2() & 0x7F; }
      constexpr std::uint8_t     ui_hint() const
                                 { return (byte3() >> 4) & 0x3; }
      constexpr std::uint8_t     midi1() const
                                 { return (byte3() >> 2) & 0x3; }
      constexpr std::uint8_t     direction() const { return byte3() & 0x3; }
      constexpr std::uint8_t     first_group() const
                                 { return (data[1] >> 24) & 0xFF; }
      constexpr std::uint8_t     groups() const
                                 { return (data[1] >> 16) & 0xFF; }
      constexpr std::uint8_t     midi_ci_version() const
                                 { return (data[1] >> 8) & 0xFF; }
      constexpr std::uint8_t     sysex8_streams() const
                                 { return data[1] & 0xFF; }
   };

   // 7.1.10, 7.1.11: the first and last events of a clip file.
   struct start_of_clip : stream_message
   {
      using stream_message::stream_message;
   };

   struct end_of_clip : stream_message
   {
      using stream_message::stream_message;
   };

   ////////////////////////////////////////////////////////////////////////////
   // The three text messages, 7.1.4, 7.1.5 and 7.1.9, may span packets and
   // are gathered by packet_reader, which hands over one of these views.
   // Like sysex_view, the text refers to the reader's buffer and is valid
   // only for the duration of the call.
   ////////////////////////////////////////////////////////////////////////////
   struct text_view : message_base
   {
      constexpr text_view(std::string_view text)
       : _text(text)
      {}

      constexpr std::string_view text() const   { return _text; }

   private:

      std::string_view  _text;
   };

   // UTF-8, at most 98 bytes.
   struct endpoint_name_view : text_view
   {
      using text_view::text_view;
   };

   // ASCII 32 to 126, at most 16 bytes; the serial number where there is one.
   struct product_instance_id_view : text_view
   {
      using text_view::text_view;
   };

   struct function_block_name_view : text_view
   {
      constexpr function_block_name_view(
         std::uint8_t block, std::string_view text)
       : text_view(text), _block(block)
      {}

      constexpr std::uint8_t     block() const     { return _block; }

   private:

      std::uint8_t   _block;
   };
}

#endif
