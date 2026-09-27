/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// The asking side of discovery. An inquiry asks what an endpoint is; the
// stream_responder in endpoint.hpp answers. The two are run against each
// other here, so what one sends is what the other reads, and the
// description the host ends up with is the one the device started from.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/inquiry.hpp>
#include <q/midi/packet_reader.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;
namespace ci = q::midi_ci;

namespace
{
   using bytes = std::vector<std::uint8_t>;

   struct synth : midi2::processor
   {
      using midi2::processor::operator();
      void operator()(midi2::note_on, std::size_t) { ++_notes; }
      int _notes = 0;
   };

   midi2::function_block const blocks[] =
   {
      {true, midi2::direction::bidirectional, 0, midi2::ui_hint::both
       , 0, 1, 0x02, 0, "Keys"}
    , {true, midi2::direction::output, 0, midi2::ui_hint::sender
       , 1, 2, 0x02, 0, "Strings"}
   };

   // A host and a device joined by nothing but their packets.
   struct wire
   {
      wire(std::span<midi2::function_block const> fb
            = std::span<midi2::function_block const>{blocks})
       : _device{
            "Q Endpoint", "SN-001", {0x002109, 0x0102, 0x0304, 0x01020304}
          , true, true, false, false, midi2::protocol::midi2, true, fb}
      {}

      // One round: the host asks, the device answers, the host reads.
      void round(std::size_t time)
      {
         std::vector<midi2::packet> to_device;
         _inquiry.poll(
            [&](midi2::packet const& p) { to_device.push_back(p); }, time);
         _asked += to_device.size();

         std::vector<midi2::packet> to_host;
         for (auto const& p : to_device)
         {
            midi2::stream_responder responder{
               _device, [&](midi2::packet const& q) { to_host.push_back(q); }
             , std::ref(_device_synth)};
            _device_reader(p, time, responder);
         }

         for (auto const& p : to_host)
            _host_reader(p, time, _inquiry);
      }

      void run(int rounds = 8)
      {
         for (int i = 0; i != rounds && !_inquiry.complete(); ++i)
            round(std::size_t(i));
      }

      midi2::endpoint_description         _device;
      synth                               _device_synth;
      midi2::packet_reader<>              _device_reader;

      synth                               _host_synth;
      midi2::endpoint_inquiry<synth&>     _inquiry{_host_synth};
      midi2::packet_reader<>              _host_reader;
      std::size_t                         _asked = 0;
   };
}

TEST_CASE("7.1.1 The inquiry asks for all five replies")
{
   auto const p = midi2::make_endpoint_discovery();

   CHECK(p.message_type() == midi2::message_type::stream);
   CHECK(midi2::stream_message{p}.status()
      == midi2::stream_status::endpoint_discovery);
   CHECK(midi2::endpoint_discovery{p}.filter() == 0x1F);

   midi2::endpoint_discovery const msg{p};
   CHECK(msg.wants_info());
   CHECK(msg.wants_device_identity());
   CHECK(msg.wants_name());
   CHECK(msg.wants_product_instance_id());
   CHECK(msg.wants_stream_configuration());
}

TEST_CASE("7.1.1 A narrower filter asks for less")
{
   auto const p = midi2::make_endpoint_discovery(
      midi2::discovery_filter::info
       | midi2::discovery_filter::stream_configuration);

   midi2::endpoint_discovery const msg{p};
   CHECK(msg.wants_info());
   CHECK(!msg.wants_device_identity());
   CHECK(!msg.wants_name());
   CHECK(!msg.wants_product_instance_id());
   CHECK(msg.wants_stream_configuration());
}

TEST_CASE("7.1.7 A block inquiry names one block or all of them")
{
   auto const one = midi2::make_function_block_discovery(3);
   midi2::function_block_discovery const a{one};
   CHECK(a.block() == 3);
   CHECK(a.wants_info());
   CHECK(a.wants_name());

   auto const every = midi2::make_function_block_discovery(
      midi2::function_block_discovery::all, midi2::block_filter::info);
   midi2::function_block_discovery const b{every};
   CHECK(b.block() == midi2::function_block_discovery::all);
   CHECK(b.wants_info());
   CHECK(!b.wants_name());
}

TEST_CASE("The host ends up with the description the device started from")
{
   wire l;
   l.run();

   REQUIRE(l._inquiry.complete());
   auto const d = l._inquiry.description();

   CHECK(d.name == l._device.name);
   CHECK(d.product_instance_id == l._device.product_instance_id);
   CHECK(d.identity.manufacturer == l._device.identity.manufacturer);
   CHECK(d.identity.family == l._device.identity.family);
   CHECK(d.identity.model == l._device.identity.model);
   CHECK(d.identity.revision == l._device.identity.revision);
   CHECK(d.midi2 == l._device.midi2);
   CHECK(d.midi1 == l._device.midi1);
   CHECK(d.protocol == l._device.protocol);
   CHECK(d.static_function_blocks == l._device.static_function_blocks);

   REQUIRE(d.blocks.size() == l._device.blocks.size());
   for (std::size_t i = 0; i != d.blocks.size(); ++i)
   {
      CHECK(d.blocks[i].name == l._device.blocks[i].name);
      CHECK(d.blocks[i].active == l._device.blocks[i].active);
      CHECK(d.blocks[i].direction == l._device.blocks[i].direction);
      CHECK(d.blocks[i].first_group == l._device.blocks[i].first_group);
      CHECK(d.blocks[i].groups == l._device.blocks[i].groups);
      CHECK(d.blocks[i].midi_ci_version
         == l._device.blocks[i].midi_ci_version);
   }
}

TEST_CASE("It takes two questions, because the count is in the first answer")
{
   wire l;
   l.run();

   REQUIRE(l._inquiry.complete());
   CHECK(l._asked == 2);            // endpoint discovery, then block discovery
}

TEST_CASE("An endpoint with no function blocks needs only the first")
{
   wire l{std::span<midi2::function_block const>{}};
   l.run();

   REQUIRE(l._inquiry.complete());
   CHECK(l._asked == 1);
   CHECK(l._inquiry.description().blocks.empty());
}

TEST_CASE("Polling a finished inquiry asks nothing more")
{
   wire l;
   l.run();
   REQUIRE(l._inquiry.complete());

   auto const asked = l._asked;
   l.round(99);
   l.round(100);
   CHECK(l._asked == asked);
}

TEST_CASE("The time of the asking is kept, since the library has no clock")
{
   wire l;
   l.round(4242);

   CHECK(l._inquiry.started());
   CHECK(l._inquiry.started_at() == 4242);
}

TEST_CASE("Restarting asks again from the beginning")
{
   wire l;
   l.run();
   REQUIRE(l._inquiry.complete());

   l._inquiry.restart();
   CHECK(!l._inquiry.complete());
   CHECK(!l._inquiry.started());

   l._asked = 0;
   l.run();
   CHECK(l._inquiry.complete());
   CHECK(l._asked == 2);
   CHECK(l._inquiry.description().name == l._device.name);
}

TEST_CASE("A block's name and its info may arrive in either order")
{
   // The two messages that describe one block are separate, and nothing
   // orders them. Whichever lands second must not lose what the first said.
   synth s;
   midi2::endpoint_inquiry<synth&> inquiry{s};

   inquiry(
      midi2::endpoint_info{
         midi2::make_endpoint_info(true, 1, true, true, false, false)}
    , 0);

   inquiry(midi2::function_block_name_view{0, "Strings"}, 0);
   inquiry(
      midi2::function_block_info{
         midi2::make_function_block_info(0, blocks[1])}
    , 0);

   auto const d = inquiry.description();
   REQUIRE(d.blocks.size() == 1);
   CHECK(d.blocks[0].name == std::string_view{"Strings"});
   CHECK(d.blocks[0].groups == blocks[1].groups);
   CHECK(d.blocks[0].first_group == blocks[1].first_group);
}

TEST_CASE("After restarting it waits for real answers, not the old ones")
{
   wire w;
   w.run();
   REQUIRE(w._inquiry.complete());

   w._inquiry.restart();
   CHECK(w._inquiry.description().name.empty());
   CHECK(w._inquiry.description().blocks.empty());

   // Ask, and let nothing answer. It must sit where it is.
   std::vector<midi2::packet> sent;
   auto send = [&](midi2::packet const& p) { sent.push_back(p); };
   w._inquiry.poll(send, 0);
   w._inquiry.poll(send, 1);
   w._inquiry.poll(send, 2);

   CHECK(sent.size() == 1);
   CHECK(!w._inquiry.complete());
}

TEST_CASE("What is not a stream message passes through to the synth")
{
   wire l;
   l.run();
   l._host_reader({0x40903C00u, 0xFFFF0000u}, 0, l._inquiry);

   CHECK(l._host_synth._notes == 1);
}

namespace
{
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

   // Table 6 written out by hand, the way midi_ci.cpp does, without the
   // 0xF0 and 0xF7 the sysex readers strip.
   bytes discovery_by_hand(std::uint32_t source)
   {
      bytes b{0x7E, 0x7F, 0x0D, 0x70, 0x02};
      put_muid(b, source);
      put_muid(b, ci::broadcast_muid);
      b.insert(b.end(), {0x00, 0x21, 0x09});
      put14(b, 0x0102);
      put14(b, 0x0304);
      b.insert(b.end(), {0x01, 0x02, 0x03, 0x04});
      b.push_back(0x00);
      b.insert(b.end(), {0x00, 0x04, 0x00, 0x00});
      b.push_back(0x00);
      return b;
   }
}

TEST_CASE("Table 6 A built discovery is the one the tests write by hand")
{
   ci::identity const me{0x002109, 0x0102, 0x0304, 0x01020304};
   std::array<std::uint8_t, ci::max_message> out = {};
   auto const n = ci::make_discovery(out.data(), 0x1234567, me, 0x00);

   REQUIRE(n >= 2);
   CHECK(out[0] == 0xF0);
   CHECK(out[n-1] == 0xF7);

   bytes const built{out.begin()+1, out.begin()+n-1};
   CHECK(built == discovery_by_hand(0x1234567));
}

TEST_CASE("A device answers a built discovery with its own identity")
{
   ci::identity const me{0x002109, 0x0102, 0x0304, 0x01020304};
   ci::identity const them{0x000001, 0x0005, 0x0006, 0x00000007};

   std::array<std::uint8_t, ci::max_message> out = {};
   auto const n = ci::make_discovery(out.data(), 0x1234567, me, 0x00);

   std::vector<bytes> sent;
   auto responder = ci::responder{
      them, [] { return std::uint32_t(0x0765432); }};
   responder(
      midi::sysex_view{
         q::byte_span{out.data()+1, n-2}}
    , [&](q::byte_span b)
      {
         sent.push_back({b.begin(), b.end()});
      });

   REQUIRE(sent.size() == 1);
   ci::discovery_reply_view const reply{
      q::byte_span{sent[0].data()+1, sent[0].size()-2}};

   REQUIRE(reply.valid());
   CHECK(reply.source() == responder.muid());
   CHECK(reply.destination() == 0x1234567);
   CHECK(reply.identity().manufacturer == them.manufacturer);
   CHECK(reply.identity().family == them.family);
   CHECK(reply.identity().model == them.model);
   CHECK(reply.identity().revision == them.revision);
}

////////////////////////////////////////////////////////////////////////////
// poll takes a Sink with a send member as readily as a callable.
////////////////////////////////////////////////////////////////////////////
TEST_CASE("A Sink with a send member serves poll as well as a callable")
{
   struct member_sink
   {
      void send(midi2::packet const& p) { _sent.push_back(p); }
      std::vector<midi2::packet> _sent;
   };

   wire w;
   member_sink out;
   w._inquiry.poll(out, 0);

   std::vector<midi2::packet> sent;
   wire v;
   v._inquiry.poll([&](midi2::packet const& p) { sent.push_back(p); }, 0);

   REQUIRE(out._sent.size() == 1);
   REQUIRE(sent.size() == 1);
   auto same = [](auto const& a, auto const& b)
   {
      if (a.size() != b.size()) return false;
      for (std::size_t i = 0; i != a.size(); ++i)
         for (std::size_t w = 0; w != 4; ++w)
            if (a[i].word(w) != b[i].word(w)) return false;
      return true;
   };
   CHECK(same(out._sent, sent));
}
