/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_ENDPOINT_HPP_SEPTEMBER_10_2026)
#define CYCFI_Q_MIDI_ENDPOINT_HPP_SEPTEMBER_10_2026

#include <q/midi/ump_stream.hpp>
#include <q/midi/ci.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // The answering side of the stream messages. M2-104-UM version 1.1,
   // section 7.1: what an endpoint says about itself when asked.
   //
   // A host's first act on a packet connection is Endpoint Discovery, and
   // the filter in it names up to five replies: info, device identity,
   // name, product instance id, and the stream configuration. Function
   // Block Discovery then asks about the endpoint's parts. This is the
   // reply to all of it, built from a description the device supplies.
   ////////////////////////////////////////////////////////////////////////////
   namespace direction
   {
      enum
      {
         input          = 0b01,
         output         = 0b10,
         bidirectional  = 0b11
      };
   }

   namespace ui_hint
   {
      enum
      {
         unknown  = 0b00,
         receiver = 0b01,
         sender   = 0b10,
         both     = 0b11
      };
   }

   // 7.1.8, 7.1.9: one function block, as it is declared.
   struct function_block
   {
      bool              active;
      std::uint8_t      direction;
      std::uint8_t      midi1;            // 0 none, 1 yes, 2 yes at 31.25 kb/s
      std::uint8_t      ui_hint;
      std::uint8_t      first_group;
      std::uint8_t      groups;
      std::uint8_t      midi_ci_version;  // 0 for none
      std::uint8_t      sysex8_streams;
      std::string_view  name;             // UTF-8, at most 98 bytes
   };

   // Everything the replies are built from.
   struct endpoint_description
   {
      std::string_view  name;                 // 7.1.4, at most 98 bytes
      std::string_view  product_instance_id;  // 7.1.5, ASCII, at most 16
      midi_ci::identity identity;             // 7.1.3
      bool              midi2;                // 7.1.2, protocols supported
      bool              midi1;
      bool              receives_jr;          // 7.1.2, timestamps
      bool              transmits_jr;
      std::uint8_t      protocol;             // 7.1.6, the one in use now
      bool              static_function_blocks;
      std::span<function_block const> blocks; // at most 32
   };

   ////////////////////////////////////////////////////////////////////////////
   // Builders, one packet each. Table 33 gives the layouts; the readers in
   // ump_stream.hpp are their mirror, and the tests feed one into the
   // other.
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      constexpr std::uint32_t stream_word(
         std::uint8_t form, std::uint16_t status
       , std::uint8_t b2 = 0, std::uint8_t b3 = 0)
      {
         return (std::uint32_t(message_type::stream) << 28)
            | (std::uint32_t(form & 0x3) << 26)
            | (std::uint32_t(status & 0x3FF) << 16)
            | (std::uint32_t(b2) << 8) | b3;
      }

      constexpr std::uint32_t bytes_word(
         std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d)
      {
         return (std::uint32_t(a) << 24) | (std::uint32_t(b) << 16)
            | (std::uint32_t(c) << 8) | d;
      }
   }

   // 7.1.2. This implementation is version 1.1.
   constexpr packet make_endpoint_info(
      bool static_blocks, std::uint8_t blocks
    , bool midi2, bool midi1, bool receives_jr, bool transmits_jr)
   {
      return packet{
         detail::stream_word(0, stream_status::endpoint_info, 1, 1)
       , (static_blocks? 0x80000000u : 0u)
            | (std::uint32_t(blocks & 0x7F) << 24)
            | (midi2? 0x1000u : 0u) | (midi1? 0x100u : 0u)
            | (receives_jr? 0x2u : 0u) | (transmits_jr? 0x1u : 0u)
       , 0u, 0u};
   }

   // 7.1.3: family and model least significant seven bits first.
   constexpr packet make_device_identity(midi_ci::identity const& id)
   {
      return packet{
         detail::stream_word(0, stream_status::device_identity)
       , detail::bytes_word(
            0, (id.manufacturer >> 16) & 0x7F, (id.manufacturer >> 8) & 0x7F
          , id.manufacturer & 0x7F)
       , detail::bytes_word(
            id.family & 0x7F, (id.family >> 7) & 0x7F
          , id.model & 0x7F, (id.model >> 7) & 0x7F)
       , detail::bytes_word(
            (id.revision >> 24) & 0x7F, (id.revision >> 16) & 0x7F
          , (id.revision >> 8) & 0x7F, id.revision & 0x7F)};
   }

   // 7.1.6.3.
   constexpr packet make_stream_configuration(
      std::uint8_t protocol, bool receives_jr, bool transmits_jr)
   {
      return packet{
         detail::stream_word(
            0, stream_status::stream_configuration, protocol
          , std::uint8_t((receives_jr? 0x2 : 0) | (transmits_jr? 0x1 : 0)))
       , 0u, 0u, 0u};
   }

   // 7.1.8.
   constexpr packet make_function_block_info(
      std::uint8_t index, function_block const& fb)
   {
      return packet{
         detail::stream_word(
            0, stream_status::function_block_info
          , std::uint8_t((fb.active? 0x80 : 0) | (index & 0x1F))
          , std::uint8_t(((fb.ui_hint & 0x3) << 4) | ((fb.midi1 & 0x3) << 2)
               | (fb.direction & 0x3)))
       , detail::bytes_word(
            fb.first_group, fb.groups, fb.midi_ci_version, fb.sysex8_streams)
       , 0u, 0u};
   }

   ////////////////////////////////////////////////////////////////////////////
   // send_text: a name or an id, 7.1.4, 7.1.5 and 7.1.9, as the packets
   // that carry it. Fourteen bytes fit a packet, thirteen when the first
   // byte is a function block number, and one packet marked complete, or
   // a start, continues and an end, carry the whole. A text that fills its
   // last packet exactly has no zero terminator, and one that does not is
   // padded with zeros, which is how the reader knows where it ends.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Send>
   inline void send_text(
      std::uint16_t status, std::string_view text, Send&& send
    , int block = -1)
   {
      constexpr std::size_t max_text = 98;
      auto const per_packet = std::size_t(block < 0? 14 : 13);
      auto const size = std::min(text.size(), max_text);
      auto const packets = size == 0? 1 : (size + per_packet - 1) / per_packet;

      for (std::size_t i = 0; i != packets; ++i)
      {
         std::uint8_t form = 0;
         if (packets > 1)
            form = i == 0? 1 : (i+1 == packets? 3 : 2);

         std::uint8_t bytes[14] = {};
         std::size_t n = 0;
         if (block >= 0)
            bytes[n++] = std::uint8_t(block);
         for (std::size_t j = i*per_packet; j < size && n != 14; ++j)
            bytes[n++] = std::uint8_t(text[j]);

         send(packet{
            detail::stream_word(form, status, bytes[0], bytes[1])
          , detail::bytes_word(bytes[2], bytes[3], bytes[4], bytes[5])
          , detail::bytes_word(bytes[6], bytes[7], bytes[8], bytes[9])
          , detail::bytes_word(bytes[10], bytes[11], bytes[12], bytes[13])});
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // stream_responder: a processor that wraps a processor, answering the
   // stream messages that ask something and passing everything else along.
   //
   //    midi2::stream_responder responder{description, send, my_synth};
   //    reader(packet, time, responder);   // send(packet const&)
   //
   // Endpoint Discovery gets the replies its filter asks for, in the order
   // the bits are numbered. A Stream Configuration Request is granted when
   // the endpoint supports what it asks, and answered with the current
   // state either way, 7.1.6.3. Function Block Discovery gets info and
   // name for one block or all of them, 7.1.7.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Send, typename P>
   class stream_responder
   {
   public:

                              stream_responder(
                                 endpoint_description const& d
                               , Send send, P next)
                               : _d(d)
                               , _send(std::forward<Send>(send))
                               , _next(std::forward<P>(next))
                               , _protocol(d.protocol)
                              {}

                              template <typename Message>
      void                    operator()(Message msg, std::size_t time)
                              {
                                 _next(msg, time);
                              }

      void                    operator()(
                                 endpoint_discovery msg, std::size_t time);
      void                    operator()(
                                 stream_configuration_request msg
                               , std::size_t time);
      void                    operator()(
                                 function_block_discovery msg
                               , std::size_t time);

      std::uint8_t            protocol() const { return _protocol; }

   private:

      void                    send_block(
                                 std::uint8_t index, bool info, bool name);

      endpoint_description    _d;
      Send                    _send;
      P                       _next;
      std::uint8_t            _protocol;
   };

   template <typename Send, typename P>
   stream_responder(endpoint_description const&, Send&&, P&&)
      -> stream_responder<Send, P>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Send, typename P>
   inline void stream_responder<Send, P>::operator()(
      endpoint_discovery msg, std::size_t)
   {
      // 7.1.1: a newer version's extra fields are ignored, an older one's
      // are all there is; either way the filter is where it always was.
      if (msg.wants_info())
         _send(make_endpoint_info(
            _d.static_function_blocks, std::uint8_t(_d.blocks.size())
          , _d.midi2, _d.midi1, _d.receives_jr, _d.transmits_jr));

      if (msg.wants_device_identity())
         _send(make_device_identity(_d.identity));

      if (msg.wants_name())
         send_text(stream_status::endpoint_name, _d.name, _send);

      if (msg.wants_product_instance_id())
         send_text(stream_status::product_instance_id
                 , _d.product_instance_id.substr(0, 16), _send);

      if (msg.wants_stream_configuration())
         _send(make_stream_configuration(
            _protocol, _d.receives_jr, _d.transmits_jr));
   }

   template <typename Send, typename P>
   inline void stream_responder<Send, P>::operator()(
      stream_configuration_request msg, std::size_t)
   {
      // 7.1.6.2: change what can be changed, then state what is. Timestamps
      // are what the description says and no request turns them on.
      auto const wanted = msg.protocol();
      if ((wanted == protocol::midi1 && _d.midi1)
         || (wanted == protocol::midi2 && _d.midi2))
         _protocol = wanted;

      _send(make_stream_configuration(
         _protocol, _d.receives_jr, _d.transmits_jr));
   }

   template <typename Send, typename P>
   inline void stream_responder<Send, P>::send_block(
      std::uint8_t index, bool info, bool name)
   {
      auto const& fb = _d.blocks[index];
      if (info)
         _send(make_function_block_info(index, fb));
      if (name)
         send_text(stream_status::function_block_name, fb.name, _send, index);
   }

   template <typename Send, typename P>
   inline void stream_responder<Send, P>::operator()(
      function_block_discovery msg, std::size_t)
   {
      auto const count = _d.blocks.size();
      if (msg.block() == function_block_discovery::all)
      {
         for (std::size_t i = 0; i != count; ++i)
            send_block(std::uint8_t(i), msg.wants_info(), msg.wants_name());
      }
      else if (msg.block() < count)
      {
         send_block(msg.block(), msg.wants_info(), msg.wants_name());
      }
   }
}

#endif
