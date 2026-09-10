/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_CI_HPP_SEPTEMBER_10_2026)
#define CYCFI_Q_MIDI_CI_HPP_SEPTEMBER_10_2026

#include <q/midi/byte_reader.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace cycfi::q::midi_ci
{
   ////////////////////////////////////////////////////////////////////////////
   // MIDI Capability Inquiry, the discovery part. M2-101-UM version 1.2.
   //
   // MIDI-CI is a conversation in universal system exclusive messages, and
   // it begins the same way every time: a device sends a Discovery message
   // to the broadcast MUID, and every MIDI-CI device that hears it replies
   // with its own. That exchange is what a MIDI 2.0 host, and the
   // Association's conformance tool, do first. Profiles, property exchange
   // and process inquiry are further conversations built on it, and are
   // not here; a device that gets one is told so with a NAK.
   //
   // Every message shares a header: 0x7E, a device id, 0x0D for MIDI-CI, a
   // sub id naming the message, the format version, then the source and
   // destination MUIDs. All multi-byte fields are seven bits per byte,
   // least significant first, as sysex requires.
   ////////////////////////////////////////////////////////////////////////////
   constexpr std::uint8_t     universal = 0x7E;
   constexpr std::uint8_t     to_function_block = 0x7F;
   constexpr std::uint8_t     sub_id_midi_ci = 0x0D;

   // 5.4: version 0x02 is the 1.2 format; 0x00 is deprecated. Bit 7 is
   // reserved, and a message with it set is answered with a NAK.
   constexpr std::uint8_t     version = 0x02;

   namespace sub_id
   {
      enum
      {
         discovery         = 0x70,
         discovery_reply   = 0x71,
         ack               = 0x7D,
         invalidate_muid   = 0x7E,
         nak               = 0x7F
      };
   }

   // 5.11.2, Table 16.
   namespace nak_status
   {
      enum
      {
         nak                     = 0x00,
         not_supported           = 0x01,
         version_not_supported   = 0x02,
         not_in_use              = 0x03,
         profile_not_supported   = 0x04
      };
   }

   // 5.5.2, Table 7: the categories a device supports, as a bitmap.
   namespace category
   {
      enum
      {
         profiles          = 0x04,
         property_exchange = 0x08,
         process_inquiry   = 0x10
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // 3.3: a MUID is a 28 bit random number a device makes up at power on.
   // The top 256 values are reserved, the very last is the broadcast
   // address, and a device must never pick either.
   ////////////////////////////////////////////////////////////////////////////
   constexpr std::uint32_t    broadcast_muid = 0x0FFFFFFF;
   constexpr std::uint32_t    reserved_muid_first = 0x0FFFFF00;

   constexpr bool valid_muid(std::uint32_t muid)
   {
      return muid < reserved_muid_first;
   }

   // 3.3.3: nothing sent to the broadcast MUID may exceed 512 bytes, which
   // is also a sensible size to declare receivable.
   constexpr std::uint32_t    default_max_sysex_size = 512;

   ////////////////////////////////////////////////////////////////////////////
   // Field readers and writers. Seven bits per byte, least significant
   // byte first, which is how every multi-byte field in MIDI-CI travels.
   ////////////////////////////////////////////////////////////////////////////
   constexpr std::uint32_t read_muid(std::span<std::uint8_t const> b)
   {
      return (b[0] & 0x7F) | (std::uint32_t(b[1] & 0x7F) << 7)
         | (std::uint32_t(b[2] & 0x7F) << 14)
         | (std::uint32_t(b[3] & 0x7F) << 21);
   }

   constexpr std::uint16_t read14(std::span<std::uint8_t const> b)
   {
      return (b[0] & 0x7F) | (std::uint16_t(b[1] & 0x7F) << 7);
   }

   constexpr void write_muid(std::uint8_t* out, std::uint32_t muid)
   {
      for (int i = 0; i != 4; ++i)
         out[i] = std::uint8_t((muid >> (7*i)) & 0x7F);
   }

   constexpr void write14(std::uint8_t* out, std::uint16_t v)
   {
      out[0] = v & 0x7F;
      out[1] = (v >> 7) & 0x7F;
   }

   ////////////////////////////////////////////////////////////////////////////
   // 5.5.1: the four fields that identify a device, the same ones the
   // MIDI 1.0 device inquiry carries. The manufacturer is the three byte
   // system exclusive id, with a one byte id in the first byte and zeros
   // after. The revision's format is the device's own.
   ////////////////////////////////////////////////////////////////////////////
   struct identity
   {
      std::uint32_t  manufacturer;
      std::uint16_t  family;
      std::uint16_t  model;
      std::uint32_t  revision;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Views over a message's payload, the bytes between 0xF0 and 0xF7, as a
   // sysex_view hands them over. Each checks that it is looking at the
   // message it names and that the bytes it reads are there.
   ////////////////////////////////////////////////////////////////////////////
   struct message_view
   {
      static constexpr std::size_t header_size = 13;

      constexpr message_view(std::span<std::uint8_t const> b)
       : _b(b)
      {}

      constexpr bool             has_header() const
      {
         return _b.size() >= header_size
            && _b[0] == universal && _b[2] == sub_id_midi_ci;
      }

      constexpr std::uint8_t     device_id() const    { return _b[1]; }
      constexpr std::uint8_t     sub_id() const       { return _b[3]; }
      constexpr std::uint8_t     version() const      { return _b[4]; }
      constexpr std::uint32_t    source() const
                                 { return read_muid(_b.subspan(5)); }
      constexpr std::uint32_t    destination() const
                                 { return read_muid(_b.subspan(9)); }

   protected:

      std::span<std::uint8_t const> _b;
   };

   // Tables 6 and 8 share everything after the header, up to the reply's
   // trailing function block byte.
   struct discovery_common : message_view
   {
      using message_view::message_view;

      // Header, then 3 + 2 + 2 + 4 + 1 + 4 bytes: version 1's length.
      static constexpr std::size_t v1_size = header_size + 16;

      constexpr midi_ci::identity identity() const
      {
         return {
            (std::uint32_t(_b[13]) << 16) | (std::uint32_t(_b[14]) << 8)
               | _b[15]
          , read14(_b.subspan(16)), read14(_b.subspan(18))
          , (std::uint32_t(_b[20]) << 24) | (std::uint32_t(_b[21]) << 16)
               | (std::uint32_t(_b[22]) << 8) | _b[23]};
      }

      constexpr std::uint8_t     categories() const   { return _b[24]; }
      constexpr std::uint32_t    max_sysex_size() const
                                 { return read_muid(_b.subspan(25)); }

      // Added in version 2, and zero when a version 1 message has none.
      constexpr std::uint8_t     output_path() const
      {
         return (version() >= 0x02 && _b.size() > v1_size)? _b[29] : 0;
      }
   };

   struct discovery_view : discovery_common
   {
      using discovery_common::discovery_common;

      constexpr bool valid() const
      {
         return has_header() && sub_id() == sub_id::discovery
            && _b.size() >= v1_size;
      }
   };

   struct discovery_reply_view : discovery_common
   {
      using discovery_common::discovery_common;

      constexpr bool valid() const
      {
         return has_header() && sub_id() == sub_id::discovery_reply
            && _b.size() >= v1_size;
      }

      // Added in version 2. "Set to 0x7F if no Function Block".
      constexpr std::uint8_t     function_block() const
      {
         return (version() >= 0x02 && _b.size() > v1_size + 1)
            ? _b[30] : 0x7F;
      }
   };

   // Table 12.
   struct invalidate_muid_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header() && sub_id() == sub_id::invalidate_muid
            && _b.size() >= header_size + 4;
      }

      constexpr std::uint32_t    target() const
                                 { return read_muid(_b.subspan(13)); }
   };

   // Table 15, in its version 2 form.
   struct nak_view : message_view
   {
      using message_view::message_view;

      static constexpr std::size_t v2_size = header_size + 10;

      constexpr bool valid() const
      {
         return has_header() && sub_id() == sub_id::nak
            && _b.size() >= v2_size;
      }

      constexpr std::uint8_t     original_sub_id() const  { return _b[13]; }
      constexpr std::uint8_t     status() const           { return _b[14]; }
      constexpr std::uint8_t     status_data() const      { return _b[15]; }
      constexpr std::span<std::uint8_t const> details() const
                                 { return _b.subspan(16, 5); }
      constexpr std::uint16_t    message_length() const
                                 { return read14(_b.subspan(21)); }
      constexpr std::span<std::uint8_t const> message() const
                                 { return _b.subspan(23, message_length()); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // Builders. Each writes a whole message, 0xF0 to 0xF7, into the buffer
   // it is given and returns the length. The buffer must hold max_message
   // bytes.
   ////////////////////////////////////////////////////////////////////////////
   constexpr std::size_t max_message = 64;

   namespace detail
   {
      constexpr std::size_t header(
         std::uint8_t* out, std::uint8_t sub
       , std::uint32_t source, std::uint32_t destination)
      {
         out[0] = 0xF0;
         out[1] = universal;
         out[2] = to_function_block;
         out[3] = sub_id_midi_ci;
         out[4] = sub;
         out[5] = version;
         write_muid(out+6, source);
         write_muid(out+10, destination);
         return 14;
      }

      constexpr std::size_t identity_fields(
         std::uint8_t* out, identity const& id
       , std::uint8_t categories, std::uint32_t max_sysex_size)
      {
         out[0] = (id.manufacturer >> 16) & 0x7F;
         out[1] = (id.manufacturer >> 8) & 0x7F;
         out[2] = id.manufacturer & 0x7F;
         write14(out+3, id.family);
         write14(out+5, id.model);
         out[7] = (id.revision >> 24) & 0x7F;
         out[8] = (id.revision >> 16) & 0x7F;
         out[9] = (id.revision >> 8) & 0x7F;
         out[10] = id.revision & 0x7F;
         out[11] = categories & 0x7F;
         write_muid(out+12, max_sysex_size);
         return 16;
      }
   }

   // Table 8.
   constexpr std::size_t make_discovery_reply(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , identity const& id, std::uint8_t categories
    , std::uint32_t max_sysex_size, std::uint8_t output_path
    , std::uint8_t function_block)
   {
      auto n = detail::header(
         out, sub_id::discovery_reply, source, destination);
      n += detail::identity_fields(out+n, id, categories, max_sysex_size);
      out[n++] = output_path & 0x7F;
      out[n++] = function_block & 0x7F;
      out[n++] = 0xF7;
      return n;
   }

   // Table 12.
   constexpr std::size_t make_invalidate_muid(
      std::uint8_t* out, std::uint32_t source, std::uint32_t target)
   {
      auto n = detail::header(
         out, sub_id::invalidate_muid, source, broadcast_muid);
      write_muid(out+n, target);
      n += 4;
      out[n++] = 0xF7;
      return n;
   }

   // Table 15, with no message text.
   constexpr std::size_t make_nak(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t original_sub_id, std::uint8_t status
    , std::uint8_t status_data = 0)
   {
      auto n = detail::header(out, sub_id::nak, source, destination);
      out[n++] = original_sub_id & 0x7F;
      out[n++] = status & 0x7F;
      out[n++] = status_data & 0x7F;
      for (int i = 0; i != 5; ++i)
         out[n++] = 0;
      write14(out+n, 0);
      n += 2;
      out[n++] = 0xF7;
      return n;
   }

   ////////////////////////////////////////////////////////////////////////////
   // responder: the device's side of discovery.
   //
   //    midi_ci::responder responder{identity, random};
   //    responder(sysex_view, send);      // send(std::span<uint8_t const>)
   //
   // It answers a Discovery addressed to it or to everyone with a Reply to
   // Discovery, resolves a collision the way 5.9.1 option B says, takes a
   // new MUID when told to by an Invalidate MUID, and NAKs any other
   // MIDI-CI message sent to it. Everything else it ignores.
   //
   // Random is called for a new MUID, at construction and whenever the
   // current one is invalidated; 3.3.1 wants a good generator, and a
   // device must not reuse a MUID across restarts. Values that are
   // reserved or the broadcast address are drawn again.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Random>
   class responder
   {
   public:

                              responder(
                                 identity id, Random random
                               , std::uint8_t categories = 0
                               , std::uint32_t max_sysex_size
                                    = default_max_sysex_size)
                               : _identity(id)
                               , _random(std::move(random))
                               , _categories(categories)
                               , _max_sysex_size(max_sysex_size)
                              {
                                 new_muid();
                              }

                              template <typename Send>
      void                    operator()(
                                 midi_1_0::sysex_view msg, Send&& send);

      std::uint32_t           muid() const { return _muid; }
      void                    new_muid();

   private:

      identity                _identity;
      Random                  _random;
      std::uint8_t            _categories;
      std::uint32_t           _max_sysex_size;
      std::uint32_t           _muid = 0;
      std::array<std::uint8_t, max_message> _out = {};
   };

   template <typename Random>
   responder(identity, Random) -> responder<Random>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Random>
   inline void responder<Random>::new_muid()
   {
      do
         _muid = _random() & 0x0FFFFFFF;
      while (!valid_muid(_muid));
   }

   template <typename Random>
   template <typename Send>
   inline void responder<Random>::operator()(
      midi_1_0::sysex_view msg, Send&& send)
   {
      message_view const m{msg.data()};
      if (!m.has_header())
         return;

      auto const destination = m.destination();
      auto const to_us = destination == _muid;
      auto const to_all = destination == broadcast_muid;
      if (!to_us && !to_all)
         return;

      // 5.4: reserved version bits set means a NAK, whatever the message.
      if (m.version() & 0x80)
      {
         auto const n = make_nak(
            _out.data(), _muid, m.source(), m.sub_id()
          , nak_status::version_not_supported);
         send(std::span<std::uint8_t const>{_out.data(), n});
         return;
      }

      switch (m.sub_id())
      {
         case sub_id::discovery:
         {
            discovery_view const d{msg.data()};
            if (!d.valid())
               return;

            // 5.9.1, option B: our MUID in someone else's message is a
            // collision. Invalidate it, then take a new one.
            if (d.source() == _muid)
            {
               auto const n = make_invalidate_muid(_out.data(), _muid, _muid);
               send(std::span<std::uint8_t const>{_out.data(), n});
               new_muid();
               return;
            }

            // 4.1: reply with our MUID as the source and theirs as the
            // destination, in our own version, echoing their output path.
            auto const n = make_discovery_reply(
               _out.data(), _muid, d.source(), _identity, _categories
             , _max_sysex_size, d.output_path(), 0x7F);
            send(std::span<std::uint8_t const>{_out.data(), n});
            return;
         }

         case sub_id::invalidate_muid:
         {
            invalidate_muid_view const v{msg.data()};
            if (v.valid() && v.target() == _muid)
               new_muid();
            return;
         }

         case sub_id::discovery_reply:
         case sub_id::ack:
         case sub_id::nak:
            // Replies to inquiries we did not make.
            return;

         default:
            // 5.11: a message we do not support, and it was sent to us in
            // particular, so we say so.
            if (to_us)
            {
               auto const n = make_nak(
                  _out.data(), _muid, m.source(), m.sub_id()
                , nak_status::not_supported);
               send(std::span<std::uint8_t const>{_out.data(), n});
            }
            return;
      }
   }
}

#endif
