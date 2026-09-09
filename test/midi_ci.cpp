/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// MIDI-CI discovery. From M2-101-UM MIDI Capability Inquiry version 1.2,
// May 11 2023: sections 3.3 (MUID), 4.1, 5.4, 5.5, 5.6, 5.9 and 5.11, and
// Tables 6, 8, 12 and 15. Cases are named for their clause.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/ci.hpp>

#include <cstdint>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace ci = q::midi_ci;

namespace
{
   using bytes = std::vector<std::uint8_t>;

   // 3.3: a MUID travels as four seven bit bytes, least significant first.
   void put_muid(bytes& b, std::uint32_t muid)
   {
      for (int i = 0; i != 4; ++i)
         b.push_back(std::uint8_t((muid >> (7*i)) & 0x7F));
   }

   void put14(bytes& b, std::uint16_t v)
   {
      b.push_back(v & 0x7F);
      b.push_back((v >> 7) & 0x7F);
   }

   // Table 6, without the 0xF0 and 0xF7 the sysex readers strip.
   bytes discovery(
      std::uint32_t source, std::uint32_t destination
    , std::uint8_t version = 0x02, std::uint8_t path = 0x00)
   {
      bytes b{0x7E, 0x7F, 0x0D, 0x70, version};
      put_muid(b, source);
      put_muid(b, destination);
      b.insert(b.end(), {0x00, 0x21, 0x09});    // manufacturer, three bytes
      put14(b, 0x0102);                          // family
      put14(b, 0x0304);                          // model
      b.insert(b.end(), {0x01, 0x02, 0x03, 0x04});   // revision
      b.push_back(0x00);                         // categories: none
      b.insert(b.end(), {0x00, 0x04, 0x00, 0x00});   // max sysex 512
      if (version >= 0x02)
         b.push_back(path);
      return b;
   }

   struct sink
   {
      void operator()(std::span<std::uint8_t const> out)
      {
         _sent.push_back({out.begin(), out.end()});
      }
      std::vector<bytes> _sent;
   };

   // A generator that hands out what the test says, in order.
   struct scripted_random
   {
      std::vector<std::uint32_t> _values;
      std::size_t _next = 0;
      std::uint32_t operator()() { return _values[_next++ % _values.size()]; }
   };

   ci::identity const me{0x002109, 0x0102, 0x0304, 0x01020304};

   struct fixture
   {
      fixture(std::vector<std::uint32_t> muids = {0x1234567})
       : _responder{me, scripted_random{muids}}
      {}

      void receive(bytes const& b)
      {
         _responder(
            midi::sysex_view{std::span<std::uint8_t const>{b}}, _send);
      }

      ci::responder<scripted_random>   _responder;
      sink                             _send;
   };
}

// 3.3 MUID ///////////////////////////////////////////////////////////////////

TEST_CASE("3.3 A MUID is 28 bits; the top of the range is reserved")
{
   // "The values 0x0FFFFF00 to 0x0FFFFFFE are reserved. The value
   // 0x0FFFFFFF is used as a Broadcast MUID"
   CHECK(ci::valid_muid(0x00000000));
   CHECK(ci::valid_muid(0x0FFFFEFF));
   CHECK_FALSE(ci::valid_muid(0x0FFFFF00));
   CHECK_FALSE(ci::valid_muid(0x0FFFFFFE));
   CHECK_FALSE(ci::valid_muid(ci::broadcast_muid));
   CHECK_FALSE(ci::valid_muid(0x10000000));
   CHECK(ci::broadcast_muid == 0x0FFFFFFF);
}

TEST_CASE("3.3.1 A responder skips generated values that are not valid")
{
   fixture f{{ci::broadcast_muid, 0x0FFFFF00, 0x7654321}};
   CHECK(f._responder.muid() == 0x7654321);
}

TEST_CASE("3.3 A MUID is written least significant seven bits first")
{
   bytes b;
   put_muid(b, 0x0FFFFFFF);
   CHECK(b == bytes{0x7F, 0x7F, 0x7F, 0x7F});

   std::uint8_t const raw[] = {0x67, 0x0A, 0x0D, 0x09};   // 0x1234567
   CHECK(ci::read_muid(std::span<std::uint8_t const>{raw}) == 0x1234567);
}

// 5.5 Discovery //////////////////////////////////////////////////////////////

TEST_CASE("Table 6 A discovery message is read field by field")
{
   auto const b = discovery(0x1234567, ci::broadcast_muid, 0x02, 0x05);
   ci::discovery_view const m{std::span<std::uint8_t const>{b}};

   CHECK(m.valid());
   CHECK(m.version() == 0x02);
   CHECK(m.source() == 0x1234567);
   CHECK(m.destination() == ci::broadcast_muid);
   CHECK(m.identity().manufacturer == 0x002109);
   CHECK(m.identity().family == 0x0102);
   CHECK(m.identity().model == 0x0304);
   CHECK(m.identity().revision == 0x01020304);
   CHECK(m.categories() == 0);
   CHECK(m.max_sysex_size() == 512);
   CHECK(m.output_path() == 0x05);
}

TEST_CASE("5.4 A version 1 discovery has no output path, and still reads")
{
   // "the device shall be able to receive and parse all previous valid
   // versions"
   auto const b = discovery(0x1234567, ci::broadcast_muid, 0x01);
   ci::discovery_view const m{std::span<std::uint8_t const>{b}};

   CHECK(m.valid());
   CHECK(m.version() == 0x01);
   CHECK(m.output_path() == 0);
}

// 4.1, 5.6 The first transaction ////////////////////////////////////////////

TEST_CASE("4.1 A discovery to the broadcast MUID gets a reply")
{
   // "Any MIDI-CI Device that receives the Discovery Message shall act as
   // a Responder by sending a Reply to Discovery message with its own MUID
   // as the source and the Initiator's MUID as the destination."
   fixture f;
   f.receive(discovery(0x0ABCDEF, ci::broadcast_muid, 0x02, 0x05));

   REQUIRE(f._send._sent.size() == 1);
   auto const& out = f._send._sent.front();

   // Table 8, bracketed for the wire.
   CHECK(out.front() == 0xF0);
   CHECK(out.back() == 0xF7);
   CHECK(out.size() == 33);
   CHECK(out[1] == 0x7E);
   CHECK(out[2] == 0x7F);
   CHECK(out[3] == 0x0D);
   CHECK(out[4] == 0x71);
   CHECK(out[5] == 0x02);

   ci::discovery_reply_view const r{
      std::span<std::uint8_t const>{out.data()+1, out.size()-2}};
   CHECK(r.valid());
   CHECK(r.source() == 0x1234567);
   CHECK(r.destination() == 0x0ABCDEF);
   CHECK(r.identity().manufacturer == 0x002109);
   CHECK(r.identity().revision == 0x01020304);
   CHECK(r.categories() == 0);
   CHECK(r.max_sysex_size() == ci::default_max_sysex_size);
   CHECK(r.output_path() == 0x05);            // echoed from the inquiry
   CHECK(r.function_block() == 0x7F);         // none: "Set to 0x7F"
}

TEST_CASE("5.6 A discovery addressed to our MUID gets a reply too")
{
   fixture f;
   f.receive(discovery(0x0ABCDEF, 0x1234567));
   CHECK(f._send._sent.size() == 1);
}

TEST_CASE("A discovery addressed to someone else is ignored")
{
   fixture f;
   f.receive(discovery(0x0ABCDEF, 0x0111111));
   CHECK(f._send._sent.empty());
}

TEST_CASE("5.4 A reply is sent in our version, whatever version asked")
{
   // "Device A must respond with the v3 format of the message, well
   // knowing that Device B will ignore all fields that are new"
   fixture f;
   f.receive(discovery(0x0ABCDEF, ci::broadcast_muid, 0x01));

   REQUIRE(f._send._sent.size() == 1);
   CHECK(f._send._sent.front()[5] == 0x02);
   CHECK(f._send._sent.front().size() == 33);
}

// 5.9 Collisions /////////////////////////////////////////////////////////////

TEST_CASE("5.9.1 A discovery carrying our own MUID is a collision")
{
   // Option B: "The Responder shall reply with an Invalidate MUID message
   // with the Target MUID set to the duplicated MUID. The Responder shall
   // change its own MUID to a new value."
   fixture f{{0x1234567, 0x7654321}};
   f.receive(discovery(0x1234567, ci::broadcast_muid));

   REQUIRE(f._send._sent.size() == 1);
   auto const& out = f._send._sent.front();
   CHECK(out[4] == 0x7E);                     // Table 12: Invalidate MUID

   ci::invalidate_muid_view const m{
      std::span<std::uint8_t const>{out.data()+1, out.size()-2}};
   CHECK(m.valid());
   CHECK(m.target() == 0x1234567);
   CHECK(m.destination() == ci::broadcast_muid);
   CHECK(f._responder.muid() == 0x7654321);
}

TEST_CASE("5.6.1 An invalidate naming our MUID makes us take a new one")
{
   // "If a Device receives an Invalidate MUID message with the Target MUID
   // set to the same value as its own MUID, it shall terminate any active
   // Transactions and generate a new MUID."
   fixture f{{0x1234567, 0x7654321}};
   bytes b{0x7E, 0x7F, 0x0D, 0x7E, 0x02};
   put_muid(b, 0x0ABCDEF);
   put_muid(b, ci::broadcast_muid);
   put_muid(b, 0x1234567);
   f.receive(b);

   CHECK(f._responder.muid() == 0x7654321);
   CHECK(f._send._sent.empty());
}

TEST_CASE("An invalidate naming another MUID changes nothing here")
{
   fixture f{{0x1234567, 0x7654321}};
   bytes b{0x7E, 0x7F, 0x0D, 0x7E, 0x02};
   put_muid(b, 0x0ABCDEF);
   put_muid(b, ci::broadcast_muid);
   put_muid(b, 0x0111111);
   f.receive(b);

   CHECK(f._responder.muid() == 0x1234567);
}

// 5.11 NAK ///////////////////////////////////////////////////////////////////

TEST_CASE("5.11 A MIDI-CI message we do not support gets a NAK")
{
   // "Reply to a MIDI-CI message the Device does not support": status 0x01.
   fixture f;
   bytes b{0x7E, 0x7F, 0x0D, 0x20, 0x02};     // a profile inquiry
   put_muid(b, 0x0ABCDEF);
   put_muid(b, 0x1234567);
   f.receive(b);

   REQUIRE(f._send._sent.size() == 1);
   auto const& out = f._send._sent.front();
   CHECK(out[4] == 0x7F);

   ci::nak_view const n{
      std::span<std::uint8_t const>{out.data()+1, out.size()-2}};
   CHECK(n.valid());
   CHECK(n.source() == 0x1234567);
   CHECK(n.destination() == 0x0ABCDEF);
   CHECK(n.original_sub_id() == 0x20);
   CHECK(n.status() == ci::nak_status::not_supported);
   CHECK(n.status_data() == 0);
   CHECK(n.message_length() == 0);
}

TEST_CASE("5.4 Reserved version bits set gets a NAK with status 0x02")
{
   // "A Receiver of a Discovery message (or any other message) with the
   // reserved version bits set, reply with a NAK message ... NAK Status
   // Code set to 0x02."
   fixture f;
   f.receive(discovery(0x0ABCDEF, ci::broadcast_muid, 0x82));

   REQUIRE(f._send._sent.size() == 1);
   ci::nak_view const n{std::span<std::uint8_t const>{
      f._send._sent.front().data()+1, f._send._sent.front().size()-2}};
   CHECK(n.valid());
   CHECK(n.status() == ci::nak_status::version_not_supported);
}

TEST_CASE("An unsupported message for someone else gets no NAK")
{
   fixture f;
   bytes b{0x7E, 0x7F, 0x0D, 0x20, 0x02};
   put_muid(b, 0x0ABCDEF);
   put_muid(b, 0x0111111);
   f.receive(b);

   CHECK(f._send._sent.empty());
}

// Not ours ///////////////////////////////////////////////////////////////////

TEST_CASE("A sysex that is not MIDI-CI is left alone")
{
   fixture f;
   f.receive({0x43, 0x10, 0x4C, 0x00});       // a manufacturer's own
   f.receive({0x7E, 0x7F, 0x06, 0x01});       // universal, but identity
   f.receive({0x7E});                         // too short to be anything

   CHECK(f._send._sent.empty());
}

TEST_CASE("A truncated discovery is not acted on")
{
   fixture f;
   auto b = discovery(0x0ABCDEF, ci::broadcast_muid);
   b.resize(12);
   f.receive(b);

   CHECK(f._send._sent.empty());
}
