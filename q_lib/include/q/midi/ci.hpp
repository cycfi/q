/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_CI_HPP_SEPTEMBER_10_2026)
#define CYCFI_Q_MIDI_CI_HPP_SEPTEMBER_10_2026

#include <q/midi/byte_reader.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <algorithm>
#include <utility>

namespace cycfi::q::midi_ci
{
   ////////////////////////////////////////////////////////////////////////////
   // MIDI Capability Inquiry, the discovery part. M2-101-UM version 1.2.
   //
   // A MIDI-CI device is something with a MUID, and it is not the same
   // thing as the endpoint in endpoint.hpp. On a packet connection a MUID
   // belongs to a function block, so one endpoint may hold several
   // devices. On a MIDI 1.0 connection there is no endpoint at all: 3.1
   // says a device using that data format has no groups and no function
   // blocks, and MIDI-CI predates the packet format. That is why nothing
   // here depends on it, and why a responder takes a sysex_view, which
   // either reader produces.
   //
   // MIDI-CI is a conversation in universal system exclusive messages, and
   // it begins the same way every time: a device sends a Discovery message
   // to the broadcast MUID, and every MIDI-CI device that receives it replies
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
   constexpr std::uint8_t     to_group = 0x7E;
   constexpr std::uint8_t     to_function_block = 0x7F;
   constexpr std::uint8_t     sub_id_midi_ci = 0x0D;

   // 5.2.1, Figure 3: the low four bits are the minor version, 0x02 for
   // the 1.2 format, and 0x00 is deprecated. Above them are three reserved
   // bits, and 5.3 says a message with any of those set is met with a
   // NAK. The top bit is the status bit of a MIDI data byte and is always
   // zero, so it is not one of them.
   constexpr std::uint8_t     version = 0x02;
   constexpr std::uint8_t     version_reserved = 0x70;

   namespace sub_id
   {
      enum
      {
         // Category 4, process inquiry, added in version 1.2.
         process_capabilities       = 0x40,
         process_capabilities_reply = 0x41,
         message_report             = 0x42,
         message_report_reply       = 0x43,
         message_report_end         = 0x44,

         // Category 7, management.
         discovery                  = 0x70,
         discovery_reply            = 0x71,
         endpoint_info              = 0x72,
         endpoint_info_reply        = 0x73,
         ack                        = 0x7D,
         invalidate_muid            = 0x7E,
         nak                        = 0x7F
      };
   }

   // 5.10.2, Table 14.
   namespace ack_status
   {
      enum
      {
         ack          = 0x00,
         // Status data is the wait in tenths of a second, up to 12.7.
         timeout_wait = 0x10,
         // 15.1 of the Common Rules for Property Exchange, which defines
         // this one rather than MIDI-CI: the chunk arrived, send the next.
         send_next    = 0x11
      };
   }

   // 5.8.3: what an Endpoint Information inquiry asks for. Only one thing
   // is defined; 0x01 to 0x7F are reserved.
   namespace endpoint_info_status
   {
      enum
      {
         product_instance_id = 0x00
      };
   }

   // 9.3, Table 42: what a device can do in the process inquiry category.
   namespace process_feature
   {
      enum
      {
         message_report = 0x01
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // 9.5: what a MIDI Message Report asks for, and what a reply promises.
   // The three bitmaps name kinds of message; the control byte says how
   // much of each to send.
   ////////////////////////////////////////////////////////////////////////////
   namespace report_control
   {
      enum
      {
         none    = 0x00,      // begin and end only, to probe the capability
         changed = 0x01,      // only what is not at its default
         full    = 0x7F
      };
   }

   namespace report_system
   {
      enum
      {
         time_code     = 0x01,
         song_position = 0x02,
         song_select   = 0x04
      };
   }

   namespace report_channel
   {
      enum
      {
         pitch_bend       = 0x01,
         control_change   = 0x02,
         registered       = 0x04,
         assignable       = 0x08,
         program_change   = 0x10,
         channel_pressure = 0x20
      };
   }

   namespace report_note
   {
      enum
      {
         notes                  = 0x01,
         poly_pressure          = 0x02,
         per_note_pitch_bend    = 0x04,
         registered_per_note    = 0x08,
         assignable_per_note    = 0x10
      };
   }

   // 5.11.2, Table 16. Below 0x20 is a failure not worth retrying, 0x20
   // and above is a notice, and 0x40 and above asks for a retry.
   namespace nak_status
   {
      enum
      {
         nak                     = 0x00,
         not_supported           = 0x01,
         version_not_supported   = 0x02,
         not_in_use              = 0x03,
         profile_not_supported   = 0x04,
         // 15.4 of the Common Rules for Property Exchange, which defines
         // this one rather than MIDI-CI: the last chunk did not arrive.
         resend_chunk            = 0x12,
         terminate_inquiry       = 0x20,
         chunks_out_of_sequence  = 0x21,
         error_retry             = 0x40,
         malformed               = 0x41,
         timeout                 = 0x42,
         // Status data is the wait in tenths of a second, up to 12.7.
         busy                    = 0x43
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
   constexpr std::uint32_t read_muid(byte_span b)
   {
      return (b[0] & 0x7F) | (std::uint32_t(b[1] & 0x7F) << 7)
         | (std::uint32_t(b[2] & 0x7F) << 14)
         | (std::uint32_t(b[3] & 0x7F) << 21);
   }

   constexpr std::uint16_t read14(byte_span b)
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
   ////////////////////////////////////////////////////////////////////////////
   // What a class declares to play the device role: everything a MIDI-CI
   // responder reports about itself that is not a reply to one message in
   // particular. The counterpart of endpoint_description, which says what
   // a class is as an endpoint.
   //
   // A class that is a device provides
   //
   //    midi_ci::device_description const& device() const;
   //
   // and a responder asks it for one. A caller with no such class hands a
   // description over directly instead.
   ////////////////////////////////////////////////////////////////////////////
   struct identity
   {
      std::uint32_t  manufacturer;
      std::uint16_t  family;
      std::uint16_t  model;
      std::uint32_t  revision;
   };

   struct device_description
   {
      midi_ci::identity identity;

      // 5.5.2, Table 7: which MIDI-CI categories this implements.
      std::uint8_t      categories = 0;

      // 5.5.3: at least 128, and at least 512 to initiate a profile or
      // property transaction.
      std::uint32_t     max_sysex_size = default_max_sysex_size;

      // 5.8.3.1: ASCII 32 to 126, at most 16 bytes, and what tells this
      // device's own message echoed back from a real collision.
      std::string_view  product_instance_id;

      // 9.3, Table 42.
      std::uint8_t      process_features = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Views over a message's payload, the bytes between 0xF0 and 0xF7, as a
   // sysex_view hands them over. Each checks that it is looking at the
   // message it names and that the bytes it reads are there.
   ////////////////////////////////////////////////////////////////////////////
   struct message_view
   {

      static constexpr std::size_t header_size = 13;

      constexpr message_view(byte_span b)
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

      byte_span _b;
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
      constexpr byte_span        details() const
                                 { return _b.subspan(16, 5); }
      constexpr std::uint16_t    message_length() const
                                 { return read14(_b.subspan(21)); }
      constexpr byte_span        message() const
                                 { return _b.subspan(23, message_length()); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 5.10, Table 13: the acknowledgement, which has the same shape as the
   // NAK and says the opposite. Both were given their fields in version
   // 1.2; a version 1.1 NAK is the header and nothing else.
   ////////////////////////////////////////////////////////////////////////////
   struct ack_view : message_view
   {
      using message_view::message_view;

      static constexpr std::size_t v2_size = header_size + 10;

      constexpr bool valid() const
      {
         return has_header() && sub_id() == sub_id::ack
            && _b.size() >= v2_size;
      }

      constexpr std::uint8_t     original_sub_id() const  { return _b[13]; }
      constexpr std::uint8_t     status() const           { return _b[14]; }
      constexpr std::uint8_t     status_data() const      { return _b[15]; }
      constexpr byte_span        details() const
                                 { return _b.subspan(16, 5); }
      constexpr std::uint16_t    message_length() const
                                 { return read14(_b.subspan(21)); }
      constexpr byte_span        message() const
                                 { return _b.subspan(23, message_length()); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 5.8: Endpoint Information, which is how the product instance id is
   // asked for. 5.8.3.1 caps it at 16 bytes of ASCII between 32 and 126,
   // and it is what tells a device's own message echoed back from a real
   // collision with another of the same model.
   ////////////////////////////////////////////////////////////////////////////
   struct endpoint_info_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header() && sub_id() == sub_id::endpoint_info
            && _b.size() >= header_size + 1;
      }

      constexpr std::uint8_t     status() const    { return _b[13]; }
   };

   struct endpoint_info_reply_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header() && sub_id() == sub_id::endpoint_info_reply
            && _b.size() >= header_size + 3;
      }

      constexpr std::uint8_t     status() const    { return _b[13]; }
      constexpr std::uint16_t    length() const
                                 { return read14(_b.subspan(14)); }
      constexpr byte_span        information() const
                                 { return _b.subspan(16, length()); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 9: process inquiry, added in version 1.2. A device says which of
   // these it can do, and a MIDI Message Report then asks it to play back
   // the state it is in: which controllers are where, which notes are on.
   ////////////////////////////////////////////////////////////////////////////
   struct process_capabilities_reply_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header()
            && sub_id() == sub_id::process_capabilities_reply
            && _b.size() >= header_size + 1;
      }

      constexpr std::uint8_t     features() const  { return _b[13]; }
   };

   // 9.5, Table 43.
   struct message_report_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header() && sub_id() == sub_id::message_report
            && _b.size() >= header_size + 5;
      }

      constexpr std::uint8_t     control() const   { return _b[13]; }
      constexpr std::uint8_t     system() const    { return _b[14]; }
      constexpr std::uint8_t     channel() const   { return _b[16]; }
      constexpr std::uint8_t     note() const      { return _b[17]; }
   };

   // 9.6, Table 45: the same three bitmaps, without the control byte,
   // saying what the device will actually send.
   struct message_report_reply_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header() && sub_id() == sub_id::message_report_reply
            && _b.size() >= header_size + 4;
      }

      constexpr std::uint8_t     system() const    { return _b[13]; }
      constexpr std::uint8_t     channel() const   { return _b[15]; }
      constexpr std::uint8_t     note() const      { return _b[16]; }
   };

   ////////////////////////////////////////////////////////////////////////////
   // Builders. Each writes a whole message, 0xF0 to 0xF7, into the buffer
   // it is given and returns the length. The buffer must hold max_message
   // bytes.
   ////////////////////////////////////////////////////////////////////////////
   constexpr std::size_t max_message = 64;

   namespace detail
   {
      // 5.2: the device id names a channel, 0x00 to 0x0F, the group,
      // 0x7E, or the function block, 0x7F. It is the destination in an
      // inquiry and the source in a reply.
      constexpr std::size_t header_at(
         std::uint8_t* out, std::uint8_t device, std::uint8_t sub
       , std::uint32_t source, std::uint32_t destination)
      {
         out[0] = 0xF0;
         out[1] = universal;
         out[2] = device;
         out[3] = sub_id_midi_ci;
         out[4] = sub;
         out[5] = version;
         write_muid(out+6, source);
         write_muid(out+10, destination);
         return 14;
      }

      constexpr std::size_t header(
         std::uint8_t* out, std::uint8_t sub
       , std::uint32_t source, std::uint32_t destination)
      {
         return header_at(
            out, to_function_block, sub, source, destination);
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

   // Table 7: what an initiator sends. The same fields as the reply, with
   // no function block, addressed to everyone.
   constexpr std::size_t make_discovery(
      std::uint8_t* out, std::uint32_t source, identity const& id
    , std::uint8_t categories
    , std::uint32_t max_sysex_size = default_max_sysex_size
    , std::uint8_t output_path = 0)
   {
      auto n = detail::header(
         out, sub_id::discovery, source, broadcast_muid);
      n += detail::identity_fields(out+n, id, categories, max_sysex_size);
      out[n++] = output_path & 0x7F;
      out[n++] = 0xF7;
      return n;
   }

   // 5.10, Table 13: an acknowledgement, with no message text.
   constexpr std::size_t make_ack(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t original_sub_id, std::uint8_t status = ack_status::ack
    , std::uint8_t status_data = 0)
   {
      auto n = detail::header(out, sub_id::ack, source, destination);
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

   // 5.8, Table 9: asking what an endpoint is called.
   constexpr std::size_t make_endpoint_info(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t status = endpoint_info_status::product_instance_id)
   {
      auto n = detail::header(
         out, sub_id::endpoint_info, source, destination);
      out[n++] = status & 0x7F;
      out[n++] = 0xF7;
      return n;
   }

   // 5.8, Table 10. 5.8.3.1 caps the id at 16 bytes of ASCII 32 to 126.
   constexpr std::size_t make_endpoint_info_reply(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t status, std::string_view information)
   {
      auto n = detail::header(
         out, sub_id::endpoint_info_reply, source, destination);
      out[n++] = status & 0x7F;

      auto const size = std::min<std::size_t>(information.size(), 16);
      write14(out+n, std::uint16_t(size));
      n += 2;
      for (std::size_t i = 0; i != size; ++i)
         out[n++] = std::uint8_t(information[i]) & 0x7F;

      out[n++] = 0xF7;
      return n;
   }

   // 9.2, Table 40: asking what a device can report. 9.2 fixes the
   // destination at the function block, which is what the header writes.
   constexpr std::size_t make_process_capabilities(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination)
   {
      auto n = detail::header(
         out, sub_id::process_capabilities, source, destination);
      out[n++] = 0xF7;
      return n;
   }

   // 9.3, Table 41.
   constexpr std::size_t make_process_capabilities_reply(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t features)
   {
      auto n = detail::header(
         out, sub_id::process_capabilities_reply, source, destination);
      out[n++] = features & 0x7F;
      out[n++] = 0xF7;
      return n;
   }

   // 9.5, Table 43. The byte between the system bitmap and the channel
   // one is reserved for system messages not yet named.
   constexpr std::size_t make_message_report(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t control, std::uint8_t system
    , std::uint8_t channel, std::uint8_t note)
   {
      auto n = detail::header(
         out, sub_id::message_report, source, destination);
      out[n++] = control & 0x7F;
      out[n++] = system & 0x7F;
      out[n++] = 0;
      out[n++] = channel & 0x7F;
      out[n++] = note & 0x7F;
      out[n++] = 0xF7;
      return n;
   }

   // 9.6, Table 45: what will actually be sent, which 9.6 says may not be
   // more than was asked for.
   constexpr std::size_t make_message_report_reply(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t system, std::uint8_t channel, std::uint8_t note)
   {
      auto n = detail::header(
         out, sub_id::message_report_reply, source, destination);
      out[n++] = system & 0x7F;
      out[n++] = 0;
      out[n++] = channel & 0x7F;
      out[n++] = note & 0x7F;
      out[n++] = 0xF7;
      return n;
   }

   // 9.8, Table 46: the end of the report, and nothing else.
   constexpr std::size_t make_message_report_end(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination)
   {
      auto n = detail::header(
         out, sub_id::message_report_end, source, destination);
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
   //    midi_ci::responder responder{identity, rng};
   //    responder(sysex_view, send);      // send(byte_span)
   //
   // It replies to a Discovery addressed to it or to everyone with a Reply to
   // Discovery, resolves a collision the way 5.9.1 option B says, takes a
   // new MUID when told to by an Invalidate MUID, and NAKs any other
   // MIDI-CI message sent to it. Everything else it ignores. Whenever it
   // takes a new MUID it announces it with a Discovery to everyone, as 5.9
   // requires; announce() sends the same at start-up, which is the
   // application's moment to call it.
   //
   // It also handles the two inquiries whose reply is simply what the
   // device is: Endpoint Information, 5.8, from the product instance id it
   // was given, and Process Inquiry Capabilities, 9.2, from the features.
   // A MIDI Message Report is not one of those: handling it means playing
   // back every controller and note the device is holding, which only the
   // application knows, so a device that claims the feature handles that
   // message itself and one that does not gets a NAK.
   //
   // Random is called for a new MUID, at construction and whenever the
   // current one is invalidated; 3.3.1 wants a good generator, and a
   // device must not reuse a MUID across restarts. Values that are
   // reserved or the broadcast address are drawn again. Do not call the
   // generator `random`: POSIX has a function of that name and an
   // unqualified use of it is ambiguous.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Random>
   class responder
   {
   public:

                              responder(
                                 device_description const& d, Random random)
                               : _d(d)
                               , _random(std::move(random))
                              {
                                 new_muid();
                              }

      // A class that plays the device role says so with a device()
      // member, and this asks it for the description once.
                              template <typename Device>
                              requires requires (Device& d) { d.device(); }
                              responder(Device& source, Random random)
                               : _d(source.device())
                               , _random(std::move(random))
                              {
                                 new_muid();
                              }

                              responder(
                                 identity id, Random random
                               , std::uint8_t categories = 0
                               , std::uint32_t max_sysex_size
                                    = default_max_sysex_size
                               , std::string_view product_instance_id = {}
                               , std::uint8_t process_features = 0)
                               : _d{
                                    id, categories, max_sysex_size
                                  , product_instance_id, process_features}
                               , _random(std::move(random))
                              {
                                 new_muid();
                              }

                              template <typename Send>
      void                    operator()(
                                 midi_1_0::sysex_view msg, Send&& send);

      std::uint32_t           muid() const { return _muid; }
      void                    new_muid();

                              template <typename Send>
      void                    announce(Send&& send);

   private:

      device_description      _d;
      Random                  _random;
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

   // 5.5: a Discovery to the broadcast MUID, from ours, with what we are.
   template <typename Random>
   template <typename Send>
   inline void responder<Random>::announce(Send&& send)
   {
      auto const n = make_discovery(
         _out.data(), _muid, _d.identity, _d.categories, _d.max_sysex_size);
      midi::detail::emit(send, byte_span{_out.data(), n});
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

      // 5.3: reserved version bits set means a NAK, whatever the message.
      // A higher minor version is not an error: its extra fields are
      // ignored and what we know is read.
      if (m.version() & version_reserved)
      {
         auto const n = make_nak(
            _out.data(), _muid, m.source(), m.sub_id()
          , nak_status::version_not_supported);
         midi::detail::emit(send, byte_span{_out.data(), n});
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
            // collision. Invalidate it, take a new one, and say so.
            if (d.source() == _muid)
            {
               auto const n = make_invalidate_muid(_out.data(), _muid, _muid);
               midi::detail::emit(send, byte_span{_out.data(), n});
               new_muid();
               announce(send);
               return;
            }

            // 4.1: reply with our MUID as the source and theirs as the
            // destination, in our own version, echoing their output path.
            auto const n = make_discovery_reply(
               _out.data(), _muid, d.source(), _d.identity, _d.categories
             , _d.max_sysex_size, d.output_path(), 0x7F);
            midi::detail::emit(send, byte_span{_out.data(), n});
            return;
         }

         case sub_id::invalidate_muid:
         {
            invalidate_muid_view const v{msg.data()};
            if (v.valid() && v.target() == _muid)
            {
               new_muid();
               announce(send);
            }
            return;
         }

         case sub_id::endpoint_info:
         {
            if (!to_us)
               return;

            endpoint_info_view const v{msg.data()};
            if (!v.valid())
               return;

            // 5.8.3: one status is defined and the rest are reserved.
            if (v.status() != endpoint_info_status::product_instance_id)
            {
               auto const n = make_nak(
                  _out.data(), _muid, m.source(), m.sub_id()
                , nak_status::not_supported);
               midi::detail::emit(send, byte_span{_out.data(), n});
               return;
            }

            auto const n = make_endpoint_info_reply(
               _out.data(), _muid, m.source(), v.status()
             , _d.product_instance_id);
            midi::detail::emit(send, byte_span{_out.data(), n});
            return;
         }

         case sub_id::process_capabilities:
         {
            if (!to_us)
               return;

            auto const n = make_process_capabilities_reply(
               _out.data(), _muid, m.source(), _d.process_features);
            midi::detail::emit(send, byte_span{_out.data(), n});
            return;
         }

         case sub_id::message_report:
            // 9.5: the reply is the device's own state, so a device that
            // claimed the feature sends it, and this says nothing.
            if (to_us
               && !(_d.process_features & process_feature::message_report))
            {
               auto const n = make_nak(
                  _out.data(), _muid, m.source(), m.sub_id()
                , nak_status::not_supported);
               midi::detail::emit(send, byte_span{_out.data(), n});
            }
            return;

         case sub_id::discovery_reply:
         case sub_id::endpoint_info_reply:
         case sub_id::process_capabilities_reply:
         case sub_id::message_report_reply:
         case sub_id::message_report_end:
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
               midi::detail::emit(send, byte_span{_out.data(), n});
            }
            return;
      }
   }
   ////////////////////////////////////////////////////////////////////////////
   // remote_device: what a device says about itself in a Reply to
   // Discovery, Table 8, or in its own Discovery, Table 7, which has no
   // function block.
   ////////////////////////////////////////////////////////////////////////////
   struct remote_device
   {
      std::uint32_t     muid = 0;
      midi_ci::identity identity = {};
      std::uint8_t      categories = 0;
      std::uint32_t     max_sysex_size = 0;
      std::uint8_t      output_path = 0;
      std::uint8_t      function_block = 0x7F;
   };

   // 3 seconds in nanoseconds, q_io's time unit.
   constexpr std::size_t default_reply_window = 3'000'000'000;

   ////////////////////////////////////////////////////////////////////////////
   // initiator: the asking side of discovery.
   //
   //    midi_ci::initiator inquiry{my_responders};
   //    inquiry.poll(send, time);            // sends Discovery when due
   //    inquiry(sysex_view, send);           // replies arrive here
   //    for (auto const& d : inquiry.devices())
   //       use(d);
   //
   // It sits in front of the responder chain and shares its MUID, since a
   // device has one: Next provides muid() and announce(send), which send
   // the Discovery this asks with. Each Reply to Discovery addressed to us
   // adds or refreshes a device, and so does a Discovery another device
   // sends; an Invalidate MUID naming one removes it. A reply carrying our
   // own MUID is a collision, 5.9.1: this invalidates the MUID and passes
   // that on, so the responder takes a new one and asks again from it.
   // Everything else goes on to Next.
   //
   // Q has no clock, so the caller passes the time to poll. A round ends
   // when the reply window has passed, and a device that did not reply in
   // it is removed. restart begins another round. Next is told of devices
   // added and removed if it has device_added(remote_device const&) and
   // device_removed(muid). MaxDevices bounds the table; replies from
   // devices beyond it are counted and dropped.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Next, std::size_t MaxDevices = 16>
   class initiator
   {
   public:

      using device_list = std::span<remote_device const>;

      explicit                initiator(
                                 Next next
                               , std::size_t reply_window
                                    = default_reply_window)
                               : _next(std::forward<Next>(next))
                               , _window(reply_window)
                              {}

                              template <typename Send>
      void                    poll(Send&& send, std::size_t time);

                              template <typename Send>
      void                    operator()(
                                 midi_1_0::sysex_view msg, Send&& send);

      device_list             devices() const
                              { return {_devices.data(), _count}; }
      std::size_t             dropped() const { return _dropped; }

      bool                    started() const
                              { return _state != state::idle; }
      bool                    complete() const
                              { return _state == state::done; }
      void                    restart();

      // Forwarded so that a chain can be built on top of this one.
      std::uint32_t           muid() const { return _next.muid(); }

                              template <typename Send>
      void                    announce(Send&& send) { _next.announce(send); }

   private:

      enum class state : std::uint8_t { idle, waiting, done };

      void                    found(remote_device const& d);
      void                    lost(std::uint32_t muid);
      void                    erase(std::size_t i);

      Next                    _next;
      std::size_t             _window;
      std::size_t             _started_at = 0;
      state                   _state = state::idle;
      std::size_t             _count = 0;
      std::size_t             _dropped = 0;
      std::array<remote_device, MaxDevices> _devices = {};
      std::array<bool, MaxDevices> _seen = {};
      std::array<std::uint8_t, max_message> _out = {};
   };

   template <typename Next>
   initiator(Next&&) -> initiator<Next>;

   template <typename Next, std::size_t MaxDevices>
   template <typename Send>
   inline void initiator<Next, MaxDevices>::poll(
      Send&& send, std::size_t time)
   {
      if (_state == state::idle)
      {
         _next.announce(send);
         _started_at = time;
         _state = state::waiting;
         return;
      }

      // The round is over: whoever did not reply is gone.
      if (_state == state::waiting && time - _started_at >= _window)
      {
         for (std::size_t i = _count; i-- != 0;)
         {
            if (!_seen[i])
               erase(i);
         }
         _state = state::done;
      }
   }

   template <typename Next, std::size_t MaxDevices>
   inline void initiator<Next, MaxDevices>::restart()
   {
      _seen = {};
      _state = state::idle;
   }

   template <typename Next, std::size_t MaxDevices>
   inline void initiator<Next, MaxDevices>::found(remote_device const& d)
   {
      for (std::size_t i = 0; i != _count; ++i)
      {
         if (_devices[i].muid == d.muid)
         {
            _devices[i] = d;
            _seen[i] = true;
            return;
         }
      }

      if (_count == MaxDevices)
      {
         ++_dropped;
         return;
      }

      _devices[_count] = d;
      _seen[_count] = true;
      ++_count;
      if constexpr (requires { _next.device_added(d); })
         _next.device_added(d);
   }

   template <typename Next, std::size_t MaxDevices>
   inline void initiator<Next, MaxDevices>::lost(std::uint32_t muid)
   {
      for (std::size_t i = 0; i != _count; ++i)
      {
         if (_devices[i].muid == muid)
         {
            erase(i);
            return;
         }
      }
   }

   template <typename Next, std::size_t MaxDevices>
   inline void initiator<Next, MaxDevices>::erase(std::size_t i)
   {
      auto const muid = _devices[i].muid;
      for (auto j = i + 1; j != _count; ++j)
      {
         _devices[j-1] = _devices[j];
         _seen[j-1] = _seen[j];
      }
      --_count;
      if constexpr (requires { _next.device_removed(muid); })
         _next.device_removed(muid);
   }

   template <typename Next, std::size_t MaxDevices>
   template <typename Send>
   inline void initiator<Next, MaxDevices>::operator()(
      midi_1_0::sysex_view msg, Send&& send)
   {
      message_view const m{msg.data()};
      if (m.has_header())
      {
         auto const ours = _next.muid();
         switch (m.sub_id())
         {
            case sub_id::discovery_reply:
            {
               discovery_reply_view const r{msg.data()};
               if (!r.valid() || r.destination() != ours)
                  break;

               // 5.9.1: someone else has our MUID. The responder hears it
               // invalidated, takes a new MUID and asks again from it.
               if (r.source() == ours)
               {
                  auto const n =
                     make_invalidate_muid(_out.data(), ours, ours);
                  midi::detail::emit(send, byte_span{_out.data(), n});
                  _next(
                     midi_1_0::sysex_view{byte_span{_out.data()+1, n-2}}
                   , send);
                  return;
               }

               found({
                  r.source(), r.identity(), r.categories()
                , r.max_sysex_size(), r.output_path(), r.function_block()});
               return;
            }

            case sub_id::discovery:
            {
               discovery_view const d{msg.data()};
               if (d.valid() && d.source() != ours)
               {
                  found({
                     d.source(), d.identity(), d.categories()
                   , d.max_sysex_size(), d.output_path(), 0x7F});
               }
               break;
            }

            case sub_id::invalidate_muid:
            {
               invalidate_muid_view const v{msg.data()};
               if (v.valid())
                  lost(v.target());
               break;
            }

            default:
               break;
         }
      }
      _next(msg, send);
   }
}

#endif
