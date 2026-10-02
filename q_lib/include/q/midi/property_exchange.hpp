/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_PROPERTY_EXCHANGE_HPP_SEPTEMBER_18_2026)
#define CYCFI_Q_MIDI_PROPERTY_EXCHANGE_HPP_SEPTEMBER_18_2026

#include <q/midi/ci.hpp>
#include <charconv>

namespace cycfi::q::midi_ci
{
   ////////////////////////////////////////////////////////////////////////////
   // MIDI-CI Property Exchange, category 3. M2-101-UM version 1.2 section
   // 8 for the messages, and M2-103-UM Common Rules for MIDI-CI Property
   // Exchange version 1.2 for what they carry.
   //
   // This is how a device is asked what it has: a name goes out, a JSON
   // document comes back. The names are resources, and replying to a
   // ResourceList inquiry is the one thing 9.1 makes mandatory of a device
   // that supports any of this at all.
   //
   // Every message carries two payloads. The header is JSON and says what
   // is being asked or how it went; the property data is the payload, and
   // may be too long for one system exclusive, so it is sent in chunks.
   // The header goes in the first chunk only.
   ////////////////////////////////////////////////////////////////////////////
   namespace pe_status
   {
      enum
      {
         capabilities         = 0x30,
         capabilities_reply   = 0x31,
         get                  = 0x34,
         get_reply            = 0x35,
         set                  = 0x36,
         set_reply            = 0x37,
         subscription         = 0x38,
         subscription_reply   = 0x39,
         notify               = 0x3F
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // 7.4.1, Table 15: what a reply's status says. Below 400 is worth
   // trying again, 400 and above is not, and 500 is the device's own
   // fault.
   ////////////////////////////////////////////////////////////////////////////
   namespace pe_reply
   {
      enum
      {
         ok                      = 200,
         accepted                = 201,
         unavailable             = 341,
         bad_data                = 342,
         too_many_requests       = 343,
         bad_request             = 400,
         not_authorized          = 403,
         not_found               = 404,
         not_allowed             = 405,
         no_flow_control         = 406,
         flow_control_required   = 407,
         too_large               = 413,
         unsupported_encoding    = 415,
         invalid_version         = 445,
         internal_error          = 500
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // 7.2, 7.3, 8.6.2, 11.1: the header keys, spelled as the specification
   // spells them. 7.1.1 holds a key to camel case and twenty characters,
   // and says which one comes first: a request leads with the resource, a
   // reply with the status, a subscription with the command.
   ////////////////////////////////////////////////////////////////////////////
   namespace pe_key
   {
      constexpr std::string_view resource = "resource";
      constexpr std::string_view res_id = "resId";
      constexpr std::string_view status = "status";
      constexpr std::string_view message = "message";
      constexpr std::string_view encoding = "mutualEncoding";
      constexpr std::string_view media_type = "mediaType";
      constexpr std::string_view cache_time = "cacheTime";
      constexpr std::string_view flow_control = "flowControl";
      constexpr std::string_view set_partial = "setPartial";
      constexpr std::string_view offset = "offset";
      constexpr std::string_view limit = "limit";
      constexpr std::string_view total_count = "totalCount";
      constexpr std::string_view command = "command";
      constexpr std::string_view subscribe_id = "subscribeId";
      constexpr std::string_view ended_by = "endedBy";
   }

   // 11.1, Table 39: what a subscription message is asking for. The
   // responder names the subscription, and either end may end it.
   namespace pe_command
   {
      constexpr std::string_view start = "start";
      constexpr std::string_view partial = "partial";
      constexpr std::string_view full = "full";
      constexpr std::string_view notify = "notify";
      constexpr std::string_view end = "end";
   }

   // 6.1.6, Table 12: how the property data is encoded, spelled exactly.
   namespace pe_encoding
   {
      constexpr std::string_view ascii = "ASCII";
      constexpr std::string_view mcoded7 = "Mcoded7";
      constexpr std::string_view zlib_mcoded7 = "zlib+Mcoded7";
   }

   ////////////////////////////////////////////////////////////////////////////
   // 6.1.7: Mcoded7, which is how eight bit data crosses a seven bit wire.
   // Seven bytes become eight: the first carries the top bit of each of
   // the seven, most significant first, and the seven that follow carry
   // their low seven bits. A last group shorter than seven is sent the
   // same way, "with the sign bits occupying the most significant bits of
   // the first transmitted byte".
   //
   // 6.2.4 makes it mandatory for anything compressed, since zlib output
   // is not seven bit data.
   ////////////////////////////////////////////////////////////////////////////
   constexpr std::size_t mcoded7_size(std::size_t size)
   {
      return size + (size + 6) / 7;
   }

   constexpr std::size_t mcoded7_decoded_size(std::size_t size)
   {
      return size - (size + 7) / 8;
   }

   // Returns what was written, or zero when the output is too small.
   inline std::size_t mcoded7_encode(
      byte_span data, std::span<std::uint8_t> out)
   {
      if (out.size() < mcoded7_size(data.size()))
         return 0;

      std::size_t n = 0;
      for (std::size_t i = 0; i < data.size(); i += 7)
      {
         auto const count = std::min<std::size_t>(7, data.size() - i);
         auto const sign = n++;
         out[sign] = 0;

         for (std::size_t j = 0; j != count; ++j)
         {
            out[sign] |= std::uint8_t((data[i+j] >> 7) << (6 - j));
            out[n++] = data[i+j] & 0x7F;
         }
      }
      return n;
   }

   inline std::size_t mcoded7_decode(
      byte_span data, std::span<std::uint8_t> out)
   {
      if (out.size() < mcoded7_decoded_size(data.size()))
         return 0;

      std::size_t n = 0;
      for (std::size_t i = 0; i < data.size(); i += 8)
      {
         auto const count = std::min<std::size_t>(7, data.size() - i - 1);
         auto const sign = data[i];

         for (std::size_t j = 0; j != count; ++j)
            out[n++] = std::uint8_t(
               data[i+1+j] | (((sign >> (6 - j)) & 1) << 7));
      }
      return n;
   }

   ////////////////////////////////////////////////////////////////////////////
   // 8.7 to 8.13, Tables 33 to 39: what every property exchange message
   // but the capabilities pair looks like. The request id comes first,
   // then the header and its length, then the two chunk counters, then the
   // property data and its length.
   //
   // 8.3: a chunk count of zero means the sender does not know yet, and
   // the last chunk then says what it turned out to be. A chunk number of
   // zero on the last chunk means the data is not to be trusted.
   ////////////////////////////////////////////////////////////////////////////
   struct pe_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header() && _b.size() >= header_size + 9
            && _b.size() >= header_size + 9 + header_length() + length();
      }

      constexpr std::uint8_t     request_id() const   { return _b[13]; }

      constexpr std::uint16_t    header_length() const
                                 { return read14(_b.subspan(14)); }

      // Not constexpr: the bytes are read as characters.
      std::string_view           header() const
      {
         return std::string_view{
            reinterpret_cast<char const*>(_b.data() + 16), header_length()};
      }

      constexpr std::uint16_t    chunks() const
                                 { return read14(_b.subspan(16 + h())); }
      constexpr std::uint16_t    chunk() const
                                 { return read14(_b.subspan(18 + h())); }
      constexpr std::uint16_t    length() const
                                 { return read14(_b.subspan(20 + h())); }
      constexpr byte_span        data() const
                                 { return _b.subspan(22 + h(), length()); }

      // 8.3: the last chunk of a message, and whether its data is good.
      constexpr bool             last() const
                                 {
                                    return chunk() == 0
                                       || chunk() == chunks();
                                 }
      constexpr bool             good() const         { return chunk() != 0; }

   private:

      constexpr std::size_t      h() const
                                 { return header_length(); }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 8.5, 8.6, Tables 30 and 32: what each end can do. Version 1.2 added
   // the two version bytes after the count, so a version 1.1 message has
   // the count alone.
   ////////////////////////////////////////////////////////////////////////////
   struct pe_capabilities_view : message_view
   {
      using message_view::message_view;

      constexpr bool valid() const
      {
         return has_header()
            && (sub_id() == pe_status::capabilities
               || sub_id() == pe_status::capabilities_reply)
            && _b.size() >= header_size + 1;
      }

      constexpr std::uint8_t     requests() const  { return _b[13]; }

      // Zero when a version 1.1 sender left them out.
      constexpr std::uint8_t     major() const
                                 {
                                    return _b.size() >= header_size + 3
                                       ? _b[14] : 0;
                                 }
      constexpr std::uint8_t     minor() const
                                 {
                                    return _b.size() >= header_size + 3
                                       ? _b[15] : 0;
                                 }
   };

   ////////////////////////////////////////////////////////////////////////////
   // 7.1.1 holds a header to one line of JSON with no whitespace, keys in
   // camel case of at most twenty characters, and values that are a
   // number, a boolean or a string. That is narrow enough to read a value
   // out without a JSON parser, which a library that may run in an audio
   // callback has no business carrying.
   //
   // Returns what lies between the quotes of a string value, or an empty
   // view when the key is not there.
   ////////////////////////////////////////////////////////////////////////////
   constexpr std::string_view header_value(
      std::string_view header, std::string_view key)
   {
      // Look for "key":" and take what runs to the next quote.
      for (std::size_t i = 0; i + key.size() + 4 < header.size(); ++i)
      {
         if (header[i] != '"' || header.substr(i+1, key.size()) != key)
            continue;

         auto j = i + 1 + key.size();
         if (j + 2 >= header.size() || header[j] != '"' || header[j+1] != ':')
            continue;

         j += 2;
         if (header[j] != '"')
            return {};

         auto const start = ++j;
         while (j != header.size() && header[j] != '"')
            ++j;
         return header.substr(start, j - start);
      }
      return {};
   }

   ////////////////////////////////////////////////////////////////////////////
   // Builders
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      // The part every message but the capabilities pair shares.
      constexpr std::size_t pe_body(
         std::uint8_t* out, std::size_t n, std::uint8_t request
       , std::string_view header, std::uint16_t chunks, std::uint16_t chunk
       , byte_span data)
      {
         out[n++] = request & 0x7F;

         write14(out+n, std::uint16_t(header.size()));
         n += 2;
         for (auto c : header)
            out[n++] = std::uint8_t(c) & 0x7F;

         write14(out+n, chunks);
         n += 2;
         write14(out+n, chunk);
         n += 2;

         write14(out+n, std::uint16_t(data.size()));
         n += 2;
         for (auto b : data)
            out[n++] = b & 0x7F;

         out[n++] = 0xF7;
         return n;
      }
   }

   // 8.5, 8.6. The version bytes are the ones Table 31 gives for the
   // Common Rules a device follows.
   constexpr std::size_t make_pe_capabilities(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t requests, bool reply = false
    , std::uint8_t major = 0, std::uint8_t minor = 0)
   {
      auto n = detail::header(
         out
       , reply? pe_status::capabilities_reply : pe_status::capabilities
       , source, destination);
      out[n++] = requests & 0x7F;
      out[n++] = major & 0x7F;
      out[n++] = minor & 0x7F;
      out[n++] = 0xF7;
      return n;
   }

   // 8.8, Table 33: an inquiry asking for a resource. Its chunk counters
   // are both one and it carries no property data.
   constexpr std::size_t make_pe_get(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t request, std::string_view header)
   {
      auto n = detail::header(out, pe_status::get, source, destination);
      return detail::pe_body(out, n, request, header, 1, 1, {});
   }

   // 8.10, Table 35: what a device is asked to take. Property data may
   // span chunks the same way a reply's does.
   constexpr std::size_t make_pe_set(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t request, std::string_view header
    , byte_span data
    , std::uint16_t chunks = 1, std::uint16_t chunk = 1)
   {
      auto n = detail::header(out, pe_status::set, source, destination);
      return detail::pe_body(out, n, request, header, chunks, chunk, data);
   }

   // 8.11, Table 36: the confirmation, which 7.4 says carries status 200
   // and no property data.
   constexpr std::size_t make_pe_set_reply(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t request, std::string_view header)
   {
      auto n = detail::header(out, pe_status::set_reply, source, destination);
      return detail::pe_body(out, n, request, header, 1, 1, {});
   }

   // 8.9, Table 34: one chunk of a reply to a Get.
   constexpr std::size_t make_pe_get_reply(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t request, std::string_view header
    , byte_span data
    , std::uint16_t chunks = 1, std::uint16_t chunk = 1)
   {
      auto n = detail::header(out, pe_status::get_reply, source, destination);
      return detail::pe_body(out, n, request, header, chunks, chunk, data);
   }

   // 8.12, Table 37: a subscription, which either end may send, and 11.1
   // says what its command is. The responder names the subscription.
   constexpr std::size_t make_pe_subscription(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t request, std::string_view header
    , byte_span data = {})
   {
      auto n = detail::header(
         out, pe_status::subscription, source, destination);
      return detail::pe_body(out, n, request, header, 1, 1, data);
   }

   // 8.13, Table 38.
   constexpr std::size_t make_pe_subscription_reply(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t request, std::string_view header)
   {
      auto n = detail::header(
         out, pe_status::subscription_reply, source, destination);
      return detail::pe_body(out, n, request, header, 1, 1, {});
   }

   // 8.14, Table 39: 12 says the acknowledgement and the refusal have
   // taken this message's place, and a receiver still takes it.
   constexpr std::size_t make_pe_notify(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t request, std::string_view header)
   {
      auto n = detail::header(out, pe_status::notify, source, destination);
      return detail::pe_body(out, n, request, header, 1, 1, {});
   }

   ////////////////////////////////////////////////////////////////////////////
   // 15: flow control, where the receiver of a long reply says when it is
   // ready for the next piece. 5.10.3 puts the request id and the chunk
   // number in the five detail bytes of the acknowledgement, which is what
   // ties one to its transaction.
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      constexpr std::size_t pe_flow(
         std::uint8_t* out, std::uint8_t sub, std::uint32_t source
       , std::uint32_t destination, std::uint8_t original_sub_id
       , std::uint8_t status, std::uint8_t request, std::uint16_t chunk)
      {
         auto n = detail::header(out, sub, source, destination);
         out[n++] = original_sub_id & 0x7F;
         out[n++] = status & 0x7F;
         out[n++] = 0;                          // status data, reserved

         out[n++] = request & 0x7F;
         write14(out+n, chunk);
         n += 2;
         out[n++] = 0;
         out[n++] = 0;

         write14(out+n, 0);                     // no message text
         n += 2;
         out[n++] = 0xF7;
         return n;
      }
   }

   // 15.1, Table 94: the chunk arrived, send the next.
   constexpr std::size_t make_pe_ack(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t original_sub_id, std::uint8_t request
    , std::uint16_t chunk, std::uint8_t status = ack_status::send_next)
   {
      return detail::pe_flow(
         out, sub_id::ack, source, destination, original_sub_id, status
       , request, chunk);
   }

   // 15.4, Table 97: the last chunk did not arrive, send it again. The
   // chunk number is the last one that did.
   constexpr std::size_t make_pe_nak(
      std::uint8_t* out, std::uint32_t source, std::uint32_t destination
    , std::uint8_t original_sub_id, std::uint8_t request
    , std::uint16_t chunk, std::uint8_t status = nak_status::resend_chunk)
   {
      return detail::pe_flow(
         out, sub_id::nak, source, destination, original_sub_id, status
       , request, chunk);
   }

   // 5.10.3, 5.11.3: how to read those five bytes back.
   constexpr std::uint8_t pe_details_request(
      byte_span details)
   {
      return details[0];
   }

   constexpr std::uint16_t pe_details_chunk(
      byte_span details)
   {
      return read14(details.subspan(1));
   }

   ////////////////////////////////////////////////////////////////////////////
   // send_property: a reply to a Get as the chunks that carry it, the
   // counterpart of send_sysex7 for a payload that is too long for one
   // message. 8.3.1 puts the header in the first chunk and nowhere else,
   // and 8.3 counts chunks from one.
   //
   // The buffer is the caller's and bounds a chunk: 5.5.3 asks for at
   // least 512 bytes of a device that does any of this.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Send>
   inline void send_property(
      std::span<std::uint8_t> buffer, std::uint32_t source
    , std::uint32_t destination, std::uint8_t request
    , std::string_view header, byte_span data
    , Send&& send, std::uint8_t sub = pe_status::get_reply)
   {
      // The header, the three counters, the request id and the framing.
      constexpr std::size_t fixed = 14 + 1 + 2 + 2 + 2 + 2 + 1;
      auto const first = fixed + header.size();
      if (buffer.size() <= first)
         return;

      auto const room_first = buffer.size() - first;
      auto const room_rest = buffer.size() - fixed;

      auto const size = data.size();
      std::size_t chunks = 1;
      if (size > room_first)
         chunks = 1 + (size - room_first + room_rest - 1) / room_rest;

      std::size_t offset = 0;
      for (std::size_t i = 0; i != chunks; ++i)
      {
         auto const room = i == 0? room_first : room_rest;
         auto const count = std::min(room, size - offset);

         auto n = detail::header(buffer.data(), sub, source, destination);
         n = detail::pe_body(
            buffer.data(), n, request
          , i == 0? header : std::string_view{}
          , std::uint16_t(chunks), std::uint16_t(i + 1)
          , data.subspan(offset, count));

         midi::detail::emit(send, byte_span{buffer.data(), n});
         offset += count;
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // 7.4: a reply says how it went, and the header that says so is one
   // property and a number. Small enough to write without a JSON writer.
   ////////////////////////////////////////////////////////////////////////////
   inline std::string_view status_header(std::span<char> buffer, int status)
   {
      constexpr std::string_view open = R"({"status":)";
      if (buffer.size() < open.size() + 5)
         return {};

      std::copy(open.begin(), open.end(), buffer.begin());
      auto const r = std::to_chars(
         buffer.data() + open.size(), buffer.data() + buffer.size() - 1
       , status);
      *r.ptr = '}';
      return std::string_view{
         buffer.data(), std::size_t(r.ptr + 1 - buffer.data())};
   }

   ////////////////////////////////////////////////////////////////////////////
   // 11.1: a subscription names itself, and 7.1.1 says the first property
   // of a reply is the status and of a subscription the command. Both are
   // one line of scalars, so both are written without a JSON writer.
   ////////////////////////////////////////////////////////////////////////////
   inline std::string_view subscription_reply_header(
      std::span<char> buffer, int status, std::string_view id)
   {
      auto const head = status_header(buffer, status);
      if (head.empty() || id.empty())
         return head;

      constexpr std::string_view key = R"(,"subscribeId":")";
      auto const size = head.size() - 1 + key.size() + id.size() + 2;
      if (buffer.size() < size)
         return head;

      auto* p = buffer.data() + head.size() - 1;      // over the brace
      p = std::copy(key.begin(), key.end(), p);
      p = std::copy(id.begin(), id.end(), p);
      *p++ = '"';
      *p++ = '}';
      return std::string_view{buffer.data(), std::size_t(p - buffer.data())};
   }

   inline std::string_view subscription_header(
      std::span<char> buffer, std::string_view command, std::string_view id)
   {
      constexpr std::string_view a = R"({"command":")";
      constexpr std::string_view b = R"(","subscribeId":")";
      if (buffer.size() < a.size() + command.size() + b.size() + id.size() + 2)
         return {};

      auto* p = buffer.data();
      p = std::copy(a.begin(), a.end(), p);
      p = std::copy(command.begin(), command.end(), p);
      p = std::copy(b.begin(), b.end(), p);
      p = std::copy(id.begin(), id.end(), p);
      *p++ = '"';
      *p++ = '}';
      return std::string_view{buffer.data(), std::size_t(p - buffer.data())};
   }

   ////////////////////////////////////////////////////////////////////////////
   // property_responder: the device's side. It handles the capabilities
   // inquiry, handles a Get by asking the device for the named resource,
   // and hands a Set to the device to take.
   //
   //    ci::responder ci{identity, rng};
   //    ci::property_responder properties{my_device, ci};
   //
   // The device provides one member, and a second if it takes anything:
   //
   //    std::string_view property(std::string_view resource);
   //    int set_property(
   //       std::string_view resource, byte_span data);
   //
   // property returns a view into storage the device owns, which is the
   // same contract sysex_view has, and an empty view for a resource it
   // does not have, which becomes a 404. set_property returns a status
   // from pe_reply, and a device without it refuses a Set with 405.
   //
   // A device that keeps subscriptions, 11, provides three more. Q does
   // not hold the table: a header only library would have to choose how
   // many subscribers it allows, and the device already knows.
   //
   //    std::string_view subscribe(
   //       std::uint32_t muid, std::string_view resource);
   //    void unsubscribe(std::string_view id);
   //    void muid_invalidated(std::uint32_t target, bool all);
   //
   // subscribe returns the id it gave the subscription, or an empty view
   // to refuse, which becomes a 405. 11.5 ends a subscription three ways:
   // either end sends the end command, or an Invalidate MUID names one of
   // the two. That last arrives at the MIDI-CI responder rather than here,
   // so this watches for it on the way past and calls muid_invalidated,
   // with all set when the target is this device, since it then has a new
   // MUID and every subscription it held is over.
   //
   // What stays with the device is what only it can decide: that
   // something changed, and whether to send partial, full or notify. It
   // calls update for each, and Q builds and chunks the message.
   //
   // A Set in chunks, 8.3, is gathered whole before the device sees it, up
   // to SetCapacity bytes; a longer one is refused with 413. One is gathered
   // at a time, and another that starts meanwhile is refused with 343.
   //
   // 9.1 makes one resource mandatory: a device that implements any of this
   // shall reply to ResourceList. That is the device's to provide, and Q
   // does not invent it.
   ////////////////////////////////////////////////////////////////////////////
   constexpr std::string_view resource_list = "ResourceList";

   namespace detail
   {
      // 8.3: one Set gathered from its chunks. The header comes with the
      // first chunk only, so the resource name is kept beside the body.
      // A status other than zero is a refusal decided on the way, sent
      // once the last chunk is in.
      template <std::size_t Capacity>
      struct pe_set_gather
      {
         bool                 is(std::uint32_t muid, std::uint8_t id) const
                              {
                                 return active
                                    && from == muid && request == id;
                              }

         void                 start(
                                 std::uint32_t muid, std::uint8_t id
                               , std::string_view resource);
         void                 append(byte_span data);

         std::string_view     resource() const
                              { return {name.data(), name_size}; }
         byte_span            data() const
                              { return {body.data(), size}; }

         bool                 active = false;
         std::uint32_t        from = 0;
         std::uint8_t         request = 0;
         int                  status = 0;
         std::size_t          name_size = 0;
         std::size_t          size = 0;
         // The ResourceList schema caps a resource name at 36.
         std::array<char, 36> name = {};
         std::array<std::uint8_t, Capacity> body = {};
      };
   }

   template <typename Device, typename Next
           , std::size_t Capacity = default_max_sysex_size
           , std::size_t SetCapacity = 4096>
   class property_responder
   {
   public:

                              property_responder(
                                 Device device, Next next
                               , std::uint8_t requests = 1)
                               : _device(std::forward<Device>(device))
                               , _next(std::forward<Next>(next))
                               , _requests(requests)
                              {}

                              template <typename Send>
      void                    operator()(
                                 midi_1_0::sysex_view msg, Send&& send);

      // 11: one update of a subscribed resource, chunked as a reply is.
      // The command is one of pe_command, and 11.1.1 says which.
                              template <typename Send>
      void                    update(
                                 std::uint32_t to, std::string_view id
                               , std::string_view command
                               , byte_span data
                               , Send&& send);

      // The discovery responder's, forwarded so that responders nest: a
      // device with profiles and properties builds one chain of them.
      std::uint32_t           muid() const      { return _next.muid(); }

                              template <typename Send>
      void                    announce(Send&& send) { _next.announce(send); }

   private:

                              template <typename Send>
      void                    subscription(
                                 message_view const& m
                               , midi_1_0::sysex_view msg, Send& send);

                              template <typename Send>
      void                    set(
                                 message_view const& m
                               , midi_1_0::sysex_view msg, Send& send);

      int                     take(std::string_view name, byte_span data);

                              template <typename Send>
      void                    reply_status(
                                 std::uint8_t sub, std::uint32_t to
                               , std::uint8_t request, int status
                               , Send& send);

      Device                  _device;
      Next                    _next;
      std::uint8_t            _requests;
      std::array<std::uint8_t, Capacity> _out = {};
      // 11.1 caps a subscription id at eight characters, so the longest
      // header written here is a command and an id.
      std::array<char, 64>    _header = {};
      detail::pe_set_gather<SetCapacity> _set;
   };

   template <typename Device, typename Next>
   property_responder(Device&&, Next&&) -> property_responder<Device, Next>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Device, typename Next, std::size_t Capacity
           , std::size_t SetCapacity>
   template <typename Send>
   inline void
   property_responder<Device, Next, Capacity, SetCapacity>::reply_status(
      std::uint8_t sub, std::uint32_t to, std::uint8_t request, int status
    , Send& send)
   {
      auto const header = status_header(_header, status);
      auto const n = sub == pe_status::set_reply
         ? make_pe_set_reply(_out.data(), _next.muid(), to, request, header)
         : make_pe_get_reply(
              _out.data(), _next.muid(), to, request, header, {});
      midi::detail::emit(send, byte_span{_out.data(), n});
   }

   template <typename Device, typename Next, std::size_t Capacity
           , std::size_t SetCapacity>
   template <typename Send>
   inline void
   property_responder<Device, Next, Capacity, SetCapacity>::update(
      std::uint32_t to, std::string_view id, std::string_view command
    , byte_span data, Send&& send)
   {
      auto const header = subscription_header(_header, command, id);
      send_property(
         std::span<std::uint8_t>{_out}, _next.muid(), to, 0, header, data
       , send, pe_status::subscription);
   }

   template <typename Device, typename Next, std::size_t Capacity
           , std::size_t SetCapacity>
   template <typename Send>
   inline void
   property_responder<Device, Next, Capacity, SetCapacity>::subscription(
      message_view const& m, midi_1_0::sysex_view msg, Send& send)
   {
      pe_view const v{msg.data()};
      if (!v.valid())
         return;

      auto const command = header_value(v.header(), pe_key::command);
      auto const request = v.request_id();

      auto reply = [&](int status, std::string_view id)
      {
         auto const header = subscription_reply_header(_header, status, id);
         auto const n = make_pe_subscription_reply(
            _out.data(), _next.muid(), m.source(), request, header);
         midi::detail::emit(send, byte_span{_out.data(), n});
      };

      if (command == pe_command::start)
      {
         auto const resource = header_value(v.header(), pe_key::resource);
         std::string_view id;
         if constexpr (
            requires { _device.subscribe(m.source(), resource); })
         {
            id = _device.subscribe(m.source(), resource);
         }

         // 7.4.1: a resource this device will not subscribe is one the
         // request does not apply to.
         if (id.empty())
            reply(pe_reply::not_allowed, {});
         else
            reply(pe_reply::ok, id);
         return;
      }

      if (command == pe_command::end)
      {
         auto const id = header_value(v.header(), pe_key::subscribe_id);
         if constexpr (requires { _device.unsubscribe(id); })
            _device.unsubscribe(id);
         reply(pe_reply::ok, {});
         return;
      }

      // 11: partial, full and notify travel the other way, from the end
      // that holds the data to the one that asked for it.
      reply(pe_reply::not_allowed, {});
   }

   template <typename Device, typename Next, std::size_t Capacity
           , std::size_t SetCapacity>
   template <typename Send>
   inline void
   property_responder<Device, Next, Capacity, SetCapacity>::operator()(
      midi_1_0::sysex_view msg, Send&& send)
   {
      message_view const m{msg.data()};

      // 5.6.1, 11.5: an Invalidate MUID ends what that peer had under way,
      // a Set being gathered and every subscription, and all of it when it
      // names this device. It is the MIDI-CI responder's message, so this
      // only watches it go by.
      if (m.has_header() && m.sub_id() == sub_id::invalidate_muid)
      {
         invalidate_muid_view const v{msg.data()};
         if (v.valid())
         {
            auto const all = v.target() == _next.muid();
            if (all || v.target() == _set.from)
               _set.active = false;

            if constexpr (
               requires { _device.muid_invalidated(std::uint32_t{}, true); })
            {
               _device.muid_invalidated(v.target(), all);
            }
         }
      }

      auto const mine = m.has_header() && m.destination() == _next.muid()
         && m.sub_id() >= pe_status::capabilities
         && m.sub_id() <= pe_status::notify;

      if (!mine)
      {
         _next(msg, send);
         return;
      }

      switch (m.sub_id())
      {
         case pe_status::capabilities:
         {
            auto const n = make_pe_capabilities(
               _out.data(), _next.muid(), m.source(), _requests, true);
            midi::detail::emit(send, byte_span{_out.data(), n});
            return;
         }

         case pe_status::get:
         {
            pe_view const v{msg.data()};
            if (!v.valid())
               return;

            auto const name = header_value(v.header(), pe_key::resource);
            auto const body = _device.property(name);

            if (body.empty())
            {
               reply_status(
                  pe_status::get_reply, m.source(), v.request_id()
                , pe_reply::not_found, send);
               return;
            }

            auto const header = status_header(_header, pe_reply::ok);
            send_property(
               std::span<std::uint8_t>{_out}, _next.muid(), m.source()
             , v.request_id(), header
             , byte_span{
                  reinterpret_cast<std::uint8_t const*>(body.data())
                , body.size()}
             , send);
            return;
         }

         case pe_status::subscription:
            subscription(m, msg, send);
            return;

         case pe_status::set:
            set(m, msg, send);
            return;

         default:
            // Notify is superseded by the acknowledgement and the
            // refusal, and the replies are to inquiries we did not make.
            return;
      }
   }

   template <std::size_t Capacity>
   inline void detail::pe_set_gather<Capacity>::start(
      std::uint32_t muid, std::uint8_t id, std::string_view resource)
   {
      active = true;
      from = muid;
      request = id;
      status = 0;
      size = 0;

      // A name longer than any resource can have names none we have.
      name_size = std::min(resource.size(), name.size());
      std::copy_n(resource.begin(), name_size, name.begin());
      if (resource.size() > name.size())
         status = pe_reply::not_found;
   }

   template <std::size_t Capacity>
   inline void detail::pe_set_gather<Capacity>::append(byte_span data)
   {
      if (status != 0)
         return;

      // Dropped whole, never truncated.
      if (data.size() > body.size() - size)
      {
         status = pe_reply::too_large;
         return;
      }
      std::copy(data.begin(), data.end(), body.begin() + size);
      size += data.size();
   }

   // 7.4: a device that takes nothing says the resource is not applicable
   // rather than pretending it worked.
   template <typename Device, typename Next, std::size_t Capacity
           , std::size_t SetCapacity>
   inline int
   property_responder<Device, Next, Capacity, SetCapacity>::take(
      std::string_view name, byte_span data)
   {
      if constexpr (requires { _device.set_property(name, data); })
         return _device.set_property(name, data);
      else
         return pe_reply::not_allowed;
   }

   // 8.3: a Set in one message goes straight to the device. One in chunks
   // is gathered, the header from the first and the data from all, and
   // ends at the chunk whose number is the count it carries, since the
   // count may change on the way, or be zero until the last says it. A
   // chunk numbered zero ends it with nothing taken.
   template <typename Device, typename Next, std::size_t Capacity
           , std::size_t SetCapacity>
   template <typename Send>
   inline void
   property_responder<Device, Next, Capacity, SetCapacity>::set(
      message_view const& m, midi_1_0::sysex_view msg, Send& send)
   {
      pe_view const v{msg.data()};
      if (!v.valid())
         return;

      auto const from = m.source();
      auto const request = v.request_id();
      auto reply = [&](int status)
      {
         reply_status(pe_status::set_reply, from, request, status, send);
      };

      if (v.chunks() == 1 && v.chunk() == 1)
      {
         reply(take(header_value(v.header(), pe_key::resource), v.data()));
         return;
      }

      if (v.chunk() == 1)
      {
         if (_set.active && !_set.is(from, request))
         {
            reply(pe_reply::too_many_requests);
            return;
         }
         _set.start(
            from, request, header_value(v.header(), pe_key::resource));
      }
      else if (!_set.is(from, request))
      {
         return;
      }

      if (!v.good())
      {
         _set.active = false;
         return;
      }

      _set.append(v.data());
      if (v.chunk() != v.chunks())
         return;

      _set.active = false;
      if (_set.status != 0)
         reply(_set.status);
      else
         reply(take(_set.resource(), _set.data()));
   }
}

#endif
