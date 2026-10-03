/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The initiator's side of MIDI-CI discovery. From M2-101-UM MIDI
// Capability Inquiry version 1.2: sections 3.3, 5.5, 5.6 and 5.9, and
// Tables 7 and 8. Cases are named for their clause.

#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/ci.hpp>
#include <q/midi/profiles.hpp>
#include <q/midi/property_exchange.hpp>

#include <cstdint>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace ci = q::midi_ci;

namespace
{
   using bytes = std::vector<std::uint8_t>;

   struct sink
   {
      void operator()(q::byte_span out)
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
   ci::identity const them{0x7D0000, 0x0001, 0x0002, 0x00010500};

   using responder_type = ci::responder<scripted_random>;

   // The stage after the initiator: the responder, and the two hooks an
   // application provides to hear about devices coming and going.
   struct chain
   {
      chain(std::vector<std::uint32_t> muids)
       : _responder{me, scripted_random{muids}}
      {}

      template <typename Send>
      void operator()(midi::sysex_view msg, Send&& send)
      {
         _responder(msg, send);
      }

      std::uint32_t muid() const { return _responder.muid(); }

      template <typename Send>
      void announce(Send&& send) { _responder.announce(send); }

      void device_added(ci::remote_device const& d)
      {
         _added.push_back(d.muid);
      }

      void device_removed(std::uint32_t muid)
      {
         _removed.push_back(muid);
      }

      responder_type                _responder;
      std::vector<std::uint32_t>    _added;
      std::vector<std::uint32_t>    _removed;
   };

   template <std::size_t MaxDevices = 16>
   struct fixture
   {
      fixture(std::vector<std::uint32_t> muids = {0x1234567})
       : _chain{muids}
       , _initiator{_chain, 1000}
      {}

      void receive(bytes const& b)
      {
         _initiator(midi::sysex_view{q::byte_span{b}}, _send);
      }

      // What was sent, without the 0xF0 and 0xF7.
      q::byte_span sent(std::size_t i) const
      {
         auto const& b = _send._sent[i];
         return {b.data()+1, b.size()-2};
      }

      chain                                  _chain;
      ci::initiator<chain&, MaxDevices>      _initiator;
      sink                                   _send;
   };

   // Table 8: a Reply to Discovery, as the sysex readers deliver it.
   bytes reply(
      std::uint32_t source, std::uint32_t destination
    , std::uint8_t function_block = 0x7F)
   {
      std::uint8_t out[ci::max_message];
      auto const n = ci::make_discovery_reply(
         out, source, destination, them, 0x0C, 512, 0, function_block);
      return {out+1, out+n-1};
   }

   // Table 7: a Discovery, as another device sends it.
   bytes discovery(std::uint32_t source)
   {
      std::uint8_t out[ci::max_message];
      auto const n = ci::make_discovery(out, source, them, 0x0C);
      return {out+1, out+n-1};
   }

   bytes invalidate(std::uint32_t source, std::uint32_t target)
   {
      std::uint8_t out[ci::max_message];
      auto const n = ci::make_invalidate_muid(out, source, target);
      return {out+1, out+n-1};
   }
}

TEST_CASE("5.5 An initiator asks everyone once, from its own MUID")
{
   fixture f;
   CHECK(!f._initiator.started());

   f._initiator.poll(f._send, 0);
   REQUIRE(f._send._sent.size() == 1);

   ci::discovery_view const d{f.sent(0)};
   REQUIRE(d.valid());
   CHECK(d.sub_id() == ci::sub_id::discovery);
   CHECK(d.source() == 0x1234567);
   CHECK(d.destination() == ci::broadcast_muid);
   CHECK(d.identity().manufacturer == me.manufacturer);
   CHECK(f._initiator.started());

   // Polling again while the replies come in sends nothing more.
   f._initiator.poll(f._send, 10);
   CHECK(f._send._sent.size() == 1);
}

TEST_CASE("5.6 Each Reply to Discovery adds the device that sent it")
{
   fixture f;
   f._initiator.poll(f._send, 0);
   f.receive(reply(0x0AAAAAA, 0x1234567, 0x00));
   f.receive(reply(0x0BBBBBB, 0x1234567));

   auto const devices = f._initiator.devices();
   REQUIRE(devices.size() == 2);
   CHECK(devices[0].muid == 0x0AAAAAA);
   CHECK(devices[0].identity.manufacturer == them.manufacturer);
   CHECK(devices[0].identity.revision == them.revision);
   CHECK(devices[0].categories == 0x0C);
   CHECK(devices[0].supports(ci::category::profiles));
   CHECK(devices[0].supports(ci::category::property_exchange));
   CHECK(!devices[0].supports(ci::category::process_inquiry));
   CHECK(devices[0].max_sysex_size == 512);
   CHECK(devices[0].function_block == 0x00);
   CHECK(devices[1].muid == 0x0BBBBBB);
   CHECK(devices[1].function_block == 0x7F);

   // The application hears of each, and nothing was sent in reply.
   CHECK(f._chain._added == std::vector<std::uint32_t>{0x0AAAAAA, 0x0BBBBBB});
   CHECK(f._send._sent.size() == 1);
}

TEST_CASE("5.6 A second reply from the same device is not a second device")
{
   fixture f;
   f._initiator.poll(f._send, 0);
   f.receive(reply(0x0AAAAAA, 0x1234567));
   f.receive(reply(0x0AAAAAA, 0x1234567));

   CHECK(f._initiator.devices().size() == 1);
   CHECK(f._chain._added.size() == 1);
}

TEST_CASE("A reply addressed to another MUID is not ours to gather")
{
   fixture f;
   f._initiator.poll(f._send, 0);
   f.receive(reply(0x0AAAAAA, 0x0123456));

   CHECK(f._initiator.devices().empty());
   CHECK(f._send._sent.size() == 1);
}

TEST_CASE("5.5 A device announcing itself is added too")
{
   // Its Discovery says what it is, and the responder still replies.
   fixture f;
   f.receive(discovery(0x0AAAAAA));

   REQUIRE(f._initiator.devices().size() == 1);
   CHECK(f._initiator.devices()[0].muid == 0x0AAAAAA);
   CHECK(f._initiator.devices()[0].function_block == 0x7F);

   REQUIRE(f._send._sent.size() == 1);
   ci::discovery_reply_view const r{f.sent(0)};
   REQUIRE(r.valid());
   CHECK(r.destination() == 0x0AAAAAA);
}

TEST_CASE("The round is complete once the reply window has passed")
{
   fixture f;
   f._initiator.poll(f._send, 500);
   f._initiator.poll(f._send, 1499);
   CHECK(!f._initiator.complete());

   f._initiator.poll(f._send, 1500);
   CHECK(f._initiator.complete());
}

TEST_CASE("A new round forgets a device that does not reply again")
{
   fixture f;
   f._initiator.poll(f._send, 0);
   f.receive(reply(0x0AAAAAA, 0x1234567));
   f.receive(reply(0x0BBBBBB, 0x1234567));
   f._initiator.poll(f._send, 1000);
   REQUIRE(f._initiator.complete());

   f._initiator.restart();
   f._initiator.poll(f._send, 2000);
   CHECK(f._send._sent.size() == 2);            // a second Discovery
   f.receive(reply(0x0AAAAAA, 0x1234567));
   f._initiator.poll(f._send, 3000);

   REQUIRE(f._initiator.devices().size() == 1);
   CHECK(f._initiator.devices()[0].muid == 0x0AAAAAA);
   CHECK(f._chain._removed == std::vector<std::uint32_t>{0x0BBBBBB});
}

TEST_CASE("5.6.1 An Invalidate MUID naming a known device removes it")
{
   fixture f;
   f._initiator.poll(f._send, 0);
   f.receive(reply(0x0AAAAAA, 0x1234567));
   f.receive(invalidate(0x0AAAAAA, 0x0AAAAAA));

   CHECK(f._initiator.devices().empty());
   CHECK(f._chain._removed == std::vector<std::uint32_t>{0x0AAAAAA});
}

TEST_CASE("5.6.1 An Invalidate MUID naming us still reaches the responder")
{
   fixture f{{0x1234567, 0x0765432}};
   f.receive(invalidate(0x0AAAAAA, 0x1234567));

   CHECK(f._initiator.muid() == 0x0765432);
}

TEST_CASE("5.9.1 A reply carrying our own MUID is a collision")
{
   // Someone else has our MUID: invalidate it, take a new one and ask
   // again from that.
   fixture f{{0x1234567, 0x0765432}};
   f._initiator.poll(f._send, 0);
   f.receive(reply(0x1234567, 0x1234567));

   CHECK(f._initiator.devices().empty());
   CHECK(f._initiator.muid() == 0x0765432);

   REQUIRE(f._send._sent.size() == 3);
   ci::invalidate_muid_view const v{f.sent(1)};
   REQUIRE(v.valid());
   CHECK(v.source() == 0x1234567);
   CHECK(v.target() == 0x1234567);

   ci::discovery_view const d{f.sent(2)};
   REQUIRE(d.valid());
   CHECK(d.source() == 0x0765432);
   CHECK(d.destination() == ci::broadcast_muid);

   // A reply to the old MUID is no longer ours.
   f.receive(reply(0x0AAAAAA, 0x1234567));
   CHECK(f._initiator.devices().empty());
   f.receive(reply(0x0AAAAAA, 0x0765432));
   CHECK(f._initiator.devices().size() == 1);
}

TEST_CASE("A full table counts the devices it has no room for")
{
   fixture<2> f;
   f._initiator.poll(f._send, 0);
   f.receive(reply(0x0AAAAAA, 0x1234567));
   f.receive(reply(0x0BBBBBB, 0x1234567));
   f.receive(reply(0x0CCCCCC, 0x1234567));

   CHECK(f._initiator.devices().size() == 2);
   CHECK(f._initiator.dropped() == 1);
}

TEST_CASE("Messages the initiator does not gather go on to the responder")
{
   // A NAK-worthy message for us still gets the responder's NAK.
   fixture f;
   bytes unknown{0x7E, 0x7F, 0x0D, 0x05, 0x02};
   for (auto muid : {0x0AAAAAAu, 0x1234567u})
      for (int i = 0; i != 4; ++i)
         unknown.push_back(std::uint8_t((muid >> (7*i)) & 0x7F));
   f.receive(unknown);

   REQUIRE(f._send._sent.size() == 1);
   ci::nak_view const n{f.sent(0)};
   CHECK(n.valid());
}

TEST_CASE("An initiator sits in front of the whole responder chain")
{
   // Profiles and properties forward muid and announce, so the same chain
   // a device answers with is the one it asks from.
   struct device
   {
      std::string_view property(std::string_view) const { return {}; }
   };

   device dev;
   ci::profile list[1] = {{{0x7D, 0x00, 0x01, 0x01, 0x01}}};
   responder_type responder{me, scripted_random{{0x1234567}}};
   ci::profile_responder profiles{std::span<ci::profile>{list}, responder};
   ci::property_responder properties{dev, profiles};
   ci::initiator inquiry{properties, 1000};
   sink send;

   inquiry.poll(send, 0);
   REQUIRE(send._sent.size() == 1);
   ci::discovery_view const d{
      q::byte_span{send._sent[0].data()+1, send._sent[0].size()-2}};
   CHECK(d.source() == 0x1234567);

   auto const r = reply(0x0AAAAAA, 0x1234567);
   inquiry(midi::sysex_view{q::byte_span{r}}, send);
   CHECK(inquiry.devices().size() == 1);
   CHECK(inquiry.muid() == 0x1234567);
}
