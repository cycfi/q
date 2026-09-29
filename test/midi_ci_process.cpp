/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The MIDI-CI messages version 1.2 added: Endpoint Information, which is
// how the product instance id is asked for; the acknowledgement that
// mirrors the NAK; and the process inquiry category, which asks a device
// to play back the state it is holding. M2-101-UM version 1.2, sections
// 5.8, 5.10 and 9, Tables 9, 10, 13, 40, 41, 43, 45 and 46.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/ci.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace ci = q::midi_ci;

namespace
{
   using bytes = std::vector<std::uint8_t>;

   void put_muid(bytes& b, std::uint32_t muid)
   {
      for (int i = 0; i != 4; ++i)
         b.push_back(std::uint8_t((muid >> (7*i)) & 0x7F));
   }

   // A message written by hand: the header of Table 5, then a payload.
   bytes message(
      std::uint8_t sub, std::uint32_t source, std::uint32_t destination
    , bytes const& payload = {})
   {
      bytes b{0x7E, 0x7F, 0x0D, sub, 0x02};
      put_muid(b, source);
      put_muid(b, destination);
      b.insert(b.end(), payload.begin(), payload.end());
      return b;
   }

   struct sink
   {
      void operator()(q::byte_span out)
      {
         _sent.push_back({out.begin(), out.end()});
      }
      std::vector<bytes> _sent;
   };

   // The payload of what was sent, without the 0xF0 and 0xF7.
   q::byte_span body(bytes const& b)
   {
      return q::byte_span{b.data()+1, b.size()-2};
   }

   ci::identity const me{0x002109, 0x0102, 0x0304, 0x01020304};

   struct fixture
   {
      fixture(
         std::string_view id = "SN-001", std::uint8_t features = 0)
       : _responder{
            me, [] { return std::uint32_t(0x1234567); }
          , 0x00, ci::default_max_sysex_size, id, features}
      {}

      void receive(bytes const& b)
      {
         _responder(
            midi::sysex_view{q::byte_span{b}}, _send);
      }

      ci::responder<std::uint32_t(*)()>   _responder;
      sink                                _send;
   };
}

TEST_CASE("Table 9 An endpoint is asked what its product instance id is")
{
   fixture f;
   f.receive(message(ci::sub_id::endpoint_info, 0x0ABCDEF, 0x1234567, {0x00}));

   REQUIRE(f._send._sent.size() == 1);
   ci::endpoint_info_reply_view const r{body(f._send._sent.front())};

   REQUIRE(r.valid());
   CHECK(r.sub_id() == ci::sub_id::endpoint_info_reply);
   CHECK(r.source() == 0x1234567);
   CHECK(r.destination() == 0x0ABCDEF);
   CHECK(r.status() == ci::endpoint_info_status::product_instance_id);
   CHECK(r.length() == 6);

   auto const info = r.information();
   std::string const text{info.begin(), info.end()};
   CHECK(text == "SN-001");
}

TEST_CASE("5.8.3.1 A product instance id longer than sixteen bytes is cut")
{
   // "shall not be any longer than 16 bytes in size".
   fixture f{"012345678901234567890"};
   f.receive(message(ci::sub_id::endpoint_info, 0x0ABCDEF, 0x1234567, {0x00}));

   REQUIRE(f._send._sent.size() == 1);
   ci::endpoint_info_reply_view const r{body(f._send._sent.front())};
   CHECK(r.length() == 16);
}

TEST_CASE("5.8.3 A reserved endpoint information status gets a NAK")
{
   fixture f;
   f.receive(message(ci::sub_id::endpoint_info, 0x0ABCDEF, 0x1234567, {0x01}));

   REQUIRE(f._send._sent.size() == 1);
   ci::nak_view const n{body(f._send._sent.front())};
   REQUIRE(n.valid());
   CHECK(n.original_sub_id() == ci::sub_id::endpoint_info);
   CHECK(n.status() == ci::nak_status::not_supported);
}

TEST_CASE("Table 41 A device says which process inquiries it can answer")
{
   fixture f;
   f.receive(
      message(ci::sub_id::process_capabilities, 0x0ABCDEF, 0x1234567));

   REQUIRE(f._send._sent.size() == 1);
   ci::process_capabilities_reply_view const r{body(f._send._sent.front())};
   REQUIRE(r.valid());
   CHECK(r.features() == 0);

   fixture g{"SN-002", ci::process_feature::message_report};
   g.receive(
      message(ci::sub_id::process_capabilities, 0x0ABCDEF, 0x1234567));

   REQUIRE(g._send._sent.size() == 1);
   ci::process_capabilities_reply_view const s{body(g._send._sent.front())};
   CHECK(s.features() == ci::process_feature::message_report);
}

TEST_CASE("9.5 A report is refused by a device that never claimed it")
{
   fixture f;
   f.receive(
      message(
         ci::sub_id::message_report, 0x0ABCDEF, 0x1234567
       , {ci::report_control::full, 0x00, 0x00, 0x00, 0x00}));

   REQUIRE(f._send._sent.size() == 1);
   ci::nak_view const n{body(f._send._sent.front())};
   REQUIRE(n.valid());
   CHECK(n.original_sub_id() == ci::sub_id::message_report);
   CHECK(n.status() == ci::nak_status::not_supported);
}

TEST_CASE("9.5 A device that claimed it answers the report itself")
{
   // The state it would play back is the application's to know, so the
   // responder says nothing and leaves the message to it.
   fixture f{"SN-001", ci::process_feature::message_report};
   f.receive(
      message(
         ci::sub_id::message_report, 0x0ABCDEF, 0x1234567
       , {ci::report_control::full, 0x00, 0x00, 0x00, 0x00}));

   CHECK(f._send._sent.empty());
}

TEST_CASE("Table 43 A report inquiry reads back its three bitmaps")
{
   std::array<std::uint8_t, ci::max_message> out = {};
   auto const n = ci::make_message_report(
      out.data(), 0x1234567, 0x0ABCDEF, ci::report_control::changed
    , ci::report_system::song_position
    , ci::report_channel::control_change | ci::report_channel::pitch_bend
    , ci::report_note::notes | ci::report_note::per_note_pitch_bend);

   ci::message_report_view const v{
      q::byte_span{out.data()+1, n-2}};

   REQUIRE(v.valid());
   CHECK(v.control() == ci::report_control::changed);
   CHECK(v.system() == ci::report_system::song_position);
   CHECK(v.channel()
      == (ci::report_channel::control_change | ci::report_channel::pitch_bend));
   CHECK(v.note()
      == (ci::report_note::notes | ci::report_note::per_note_pitch_bend));

   // The byte between the system bitmap and the channel one is reserved.
   // The payload starts at index 14, after 0xF0 and the thirteen byte
   // header, so the reserved byte is the third of it.
   CHECK(out[16] == 0);
}

TEST_CASE("Table 45 A report reply has the bitmaps and no control byte")
{
   std::array<std::uint8_t, ci::max_message> out = {};
   auto const n = ci::make_message_report_reply(
      out.data(), 0x1234567, 0x0ABCDEF, 0x00
    , ci::report_channel::control_change, ci::report_note::notes);

   ci::message_report_reply_view const v{
      q::byte_span{out.data()+1, n-2}};

   REQUIRE(v.valid());
   CHECK(v.system() == 0);
   CHECK(v.channel() == ci::report_channel::control_change);
   CHECK(v.note() == ci::report_note::notes);

   // Four payload bytes where the inquiry has five.
   CHECK(n == 14 + 4 + 1);
}

TEST_CASE("Table 46 The end of a report is the header and nothing else")
{
   std::array<std::uint8_t, ci::max_message> out = {};
   auto const n = ci::make_message_report_end(
      out.data(), 0x1234567, 0x0ABCDEF);

   CHECK(n == 14 + 1);
   CHECK(out[4] == ci::sub_id::message_report_end);
   CHECK(out[n-1] == 0xF7);
}

TEST_CASE("Table 13 An acknowledgement has the same shape as a NAK")
{
   std::array<std::uint8_t, ci::max_message> out = {};
   auto const n = ci::make_ack(
      out.data(), 0x1234567, 0x0ABCDEF, ci::sub_id::message_report
    , ci::ack_status::timeout_wait, 20);

   ci::ack_view const v{q::byte_span{out.data()+1, n-2}};

   REQUIRE(v.valid());
   CHECK(v.sub_id() == ci::sub_id::ack);
   CHECK(v.original_sub_id() == ci::sub_id::message_report);
   CHECK(v.status() == ci::ack_status::timeout_wait);

   // 5.10.2: the wait is in multiples of 100 milliseconds, so 20 is two
   // seconds.
   CHECK(v.status_data() == 20);
   CHECK(v.message_length() == 0);
   CHECK(v.details().size() == 5);
}

TEST_CASE("Table 40 An inquiry into capabilities carries no payload")
{
   std::array<std::uint8_t, ci::max_message> out = {};
   auto const n = ci::make_process_capabilities(
      out.data(), 0x1234567, 0x0ABCDEF);

   CHECK(n == 14 + 1);
   CHECK(out[4] == ci::sub_id::process_capabilities);
}
