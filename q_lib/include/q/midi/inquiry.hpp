/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_INQUIRY_HPP_SEPTEMBER_18_2026)
#define CYCFI_Q_MIDI_INQUIRY_HPP_SEPTEMBER_18_2026

#include <q/midi/endpoint.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace cycfi::q::midi_2_0
{
   ////////////////////////////////////////////////////////////////////////////
   // The requesting side of the stream messages, the mirror of
   // endpoint.hpp.
   //
   // An endpoint reports on itself when a request arrives. Something has
   // to send one, and a host does: it is the first thing sent on a packet
   // connection.
   //
   // The replies do not come back from a function call. They cross the wire
   // and arrive later, on whatever thread the transport uses, or never, and
   // nothing marks the end of them.
   //
   // So the caller drives this, and owns the clock. Q has none, by
   // decision: it may run in an audio callback.
   ////////////////////////////////////////////////////////////////////////////

   // 7.1.1: the five replies an endpoint discovery can ask for.
   namespace discovery_filter
   {
      enum
      {
         info                 = 0x01,
         device_identity      = 0x02,
         name                 = 0x04,
         product_instance_id  = 0x08,
         stream_configuration = 0x10,
         all                  = 0x1F
      };
   }

   // 7.1.7: and the two a function block discovery can.
   namespace block_filter
   {
      enum
      {
         info = 0x1,
         name = 0x2,
         all  = 0x3
      };
   }

   // 7.1.1. This implementation is version 1.1.
   constexpr packet make_endpoint_discovery(
      std::uint8_t filter = discovery_filter::all)
   {
      return packet{
         detail::stream_word(0, stream_status::endpoint_discovery, 1, 1)
       , std::uint32_t(filter & 0x1F), 0u, 0u};
   }

   // 7.1.7: one block, or function_block_discovery::all of them.
   constexpr packet make_function_block_discovery(
      std::uint8_t block, std::uint8_t filter = block_filter::all)
   {
      return packet{
         detail::stream_word(
            0, stream_status::function_block_discovery, block
          , std::uint8_t(filter & 0x3))
       , 0u, 0u, 0u};
   }

   ////////////////////////////////////////////////////////////////////////////
   // endpoint_inquiry: a processor proxy that requests an endpoint's
   // description and gathers the replies.
   //
   //    midi2::endpoint_inquiry inquiry{my_synth};
   //    inquiry.poll(send, time);              // in the reader's thread
   //    in.process(inquiry);                   // replies arrive here
   //    if (inquiry.complete())
   //       use(inquiry.description());
   //
   // poll sends whatever is next and is safe to call as often as the
   // caller likes: it acts only when the inquiry can move. Two questions
   // are sent, not one, because the number of function blocks is in the
   // reply to the first.
   //
   // The description refers to storage inside the inquiry, so it is good
   // while the inquiry lives and until restart. MaxBlocks bounds that
   // storage; 32 is the most an endpoint may have.
   ////////////////////////////////////////////////////////////////////////////
   template <typename P, std::size_t MaxBlocks = 32>
   class endpoint_inquiry
   {
   public:

      static constexpr std::size_t max_blocks = MaxBlocks;
      static constexpr std::size_t max_name = 98;
      static constexpr std::size_t max_id = 16;

      explicit                endpoint_inquiry(P next)
                               : _next(std::forward<P>(next))
                              {}

      // The description it hands out refers to buffers in here, so a copy
      // would leave those views pointing at the original.
                              endpoint_inquiry(
                                 endpoint_inquiry const&) = delete;
      endpoint_inquiry&       operator=(
                                 endpoint_inquiry const&) = delete;

                              template <typename Send>
      void                    poll(Send&& send, std::size_t time);

      void                    operator()(
                                 endpoint_info msg, std::size_t time);
      void                    operator()(
                                 device_identity msg, std::size_t time);
      void                    operator()(
                                 endpoint_name_view msg, std::size_t time);
      void                    operator()(
                                 product_instance_id_view msg
                               , std::size_t time);
      void                    operator()(
                                 stream_configuration msg, std::size_t time);
      void                    operator()(
                                 function_block_info msg, std::size_t time);
      void                    operator()(
                                 function_block_name_view msg
                               , std::size_t time);

                              template <typename Message>
      void                    operator()(Message msg, std::size_t time)
                              {
                                 _next(msg, time);
                              }

      bool                    started() const
                              { return _state != state::idle; }
      bool                    complete() const
                              { return _state == state::done; }
      std::size_t             started_at() const
                              { return _started_at; }
      endpoint_description    description() const;
      void                    restart();

   private:

      enum class state : std::uint8_t
      {
         idle, asking_endpoint, asking_blocks, done
      };

      bool                    endpoint_replied() const
                              { return _got == discovery_filter::all; }
      bool                    blocks_replied() const;
      void                    copy_text(
                                 std::span<char> to, std::size_t& size
                               , std::string_view from);
      void                    name_block(std::size_t i);

      P                       _next;
      state                   _state = state::idle;
      std::size_t             _started_at = 0;
      std::uint8_t            _got = 0;
      std::uint32_t           _block_info = 0;
      std::uint32_t           _block_name = 0;
      std::uint8_t            _expected = 0;

      endpoint_description    _d = {};
      std::array<char, max_name>              _name = {};
      std::size_t                             _name_size = 0;
      std::array<char, max_id>                _id = {};
      std::size_t                             _id_size = 0;
      std::array<function_block, MaxBlocks>   _blocks = {};
      std::array<std::array<char, max_name>, MaxBlocks> _block_text = {};
      std::array<std::size_t, MaxBlocks>      _block_text_size = {};
   };

   template <typename P>
   endpoint_inquiry(P&&) -> endpoint_inquiry<P>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename P, std::size_t MaxBlocks>
   inline bool endpoint_inquiry<P, MaxBlocks>::blocks_replied() const
   {
      auto const want = _expected == 32
         ? 0xFFFFFFFFu : (std::uint32_t(1) << _expected) - 1;
      return (_block_info & want) == want && (_block_name & want) == want;
   }

   template <typename P, std::size_t MaxBlocks>
   inline void endpoint_inquiry<P, MaxBlocks>::copy_text(
      std::span<char> to, std::size_t& size, std::string_view from)
   {
      size = std::min(from.size(), to.size());
      std::copy_n(from.begin(), size, to.begin());
   }

   template <typename P, std::size_t MaxBlocks>
   template <typename Send>
   inline void endpoint_inquiry<P, MaxBlocks>::poll(
      Send&& send, std::size_t time)
   {
      switch (_state)
      {
         case state::idle:
            _started_at = time;
            midi::detail::emit(send, make_endpoint_discovery());
            _state = state::asking_endpoint;
            return;

         case state::asking_endpoint:
            if (!endpoint_replied())
               return;
            if (_expected == 0)
            {
               _state = state::done;
               return;
            }
            midi::detail::emit(send, make_function_block_discovery(
               function_block_discovery::all));
            _state = state::asking_blocks;
            return;

         case state::asking_blocks:
            if (blocks_replied())
               _state = state::done;
            return;

         case state::done:
            return;
      }
   }

   template <typename P, std::size_t MaxBlocks>
   inline void endpoint_inquiry<P, MaxBlocks>::operator()(
      endpoint_info msg, std::size_t time)
   {
      _d.static_function_blocks = msg.static_function_blocks();
      _d.midi2 = msg.midi2();
      _d.midi1 = msg.midi1();
      _d.receives_jr = msg.receives_jr();
      _d.transmits_jr = msg.transmits_jr();
      _expected = std::uint8_t(
         std::min<std::size_t>(msg.function_blocks(), MaxBlocks));
      _got |= discovery_filter::info;
      _next(msg, time);
   }

   template <typename P, std::size_t MaxBlocks>
   inline void endpoint_inquiry<P, MaxBlocks>::operator()(
      device_identity msg, std::size_t time)
   {
      _d.identity = {
         msg.manufacturer(), msg.family(), msg.model(), msg.revision()};
      _got |= discovery_filter::device_identity;
      _next(msg, time);
   }

   template <typename P, std::size_t MaxBlocks>
   inline void endpoint_inquiry<P, MaxBlocks>::operator()(
      endpoint_name_view msg, std::size_t time)
   {
      copy_text(_name, _name_size, msg.text());
      _got |= discovery_filter::name;
      _next(msg, time);
   }

   template <typename P, std::size_t MaxBlocks>
   inline void endpoint_inquiry<P, MaxBlocks>::operator()(
      product_instance_id_view msg, std::size_t time)
   {
      copy_text(_id, _id_size, msg.text());
      _got |= discovery_filter::product_instance_id;
      _next(msg, time);
   }

   template <typename P, std::size_t MaxBlocks>
   inline void endpoint_inquiry<P, MaxBlocks>::operator()(
      stream_configuration msg, std::size_t time)
   {
      _d.protocol = msg.protocol();
      _got |= discovery_filter::stream_configuration;
      _next(msg, time);
   }

   template <typename P, std::size_t MaxBlocks>
   inline void endpoint_inquiry<P, MaxBlocks>::operator()(
      function_block_info msg, std::size_t time)
   {
      auto const i = msg.block();
      if (i < MaxBlocks)
      {
         _blocks[i] = {
            msg.active(), msg.direction(), msg.midi1(), msg.ui_hint()
          , msg.first_group(), msg.groups(), msg.midi_ci_version()
          , msg.sysex8_streams(), {}};
         name_block(i);
         _block_info |= std::uint32_t(1) << i;
      }
      _next(msg, time);
   }

   template <typename P, std::size_t MaxBlocks>
   inline void endpoint_inquiry<P, MaxBlocks>::operator()(
      function_block_name_view msg, std::size_t time)
   {
      auto const i = msg.block();
      if (i < MaxBlocks)
      {
         copy_text(_block_text[i], _block_text_size[i], msg.text());
         name_block(i);
         _block_name |= std::uint32_t(1) << i;
      }
      _next(msg, time);
   }

   // A block's name lives in this object, and both messages that touch a
   // block set the view afresh, so neither order loses it.
   template <typename P, std::size_t MaxBlocks>
   inline void endpoint_inquiry<P, MaxBlocks>::name_block(std::size_t i)
   {
      _blocks[i].name = std::string_view{
         _block_text[i].data(), _block_text_size[i]};
   }

   template <typename P, std::size_t MaxBlocks>
   inline endpoint_description
   endpoint_inquiry<P, MaxBlocks>::description() const
   {
      auto d = _d;
      d.name = std::string_view{_name.data(), _name_size};
      d.product_instance_id = std::string_view{_id.data(), _id_size};
      d.blocks = std::span<function_block const>{_blocks.data(), _expected};
      return d;
   }

   template <typename P, std::size_t MaxBlocks>
   inline void endpoint_inquiry<P, MaxBlocks>::restart()
   {
      _state = state::idle;
      _got = 0;
      _block_info = 0;
      _block_name = 0;
      _expected = 0;
      _name_size = 0;
      _id_size = 0;
      _blocks = {};
      _block_text_size = {};
   }
}

#endif
