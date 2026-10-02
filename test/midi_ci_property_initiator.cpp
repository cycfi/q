/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The initiator's side of property exchange. From M2-101-UM section 8
// and M2-103-UM Common Rules for Property Exchange version 1.2: sections
// 7.4, 8.3, 8.5 to 8.13 and 11. Cases are named for their clause.

#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/property_exchange.hpp>

#include <cstdint>
#include <string>
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

   struct fixed_random
   {
      std::uint32_t operator()() const { return 0x1234567; }
   };

   std::string text(q::byte_span b) { return {b.begin(), b.end()}; }

   ci::identity const me{0x002109, 0x0102, 0x0304, 0x01020304};

   ci::remote_device const synth{
      0x0AAAAAA, {0x7D0000, 1, 1, 0}, 0x0C, 512, 0, 0x7F};

   // What this device answers itself, for the cases where a message is
   // not ours to handle.
   struct my_device
   {
      std::string_view property(std::string_view) const { return {}; }
   };

   struct reply
   {
      std::uint32_t     muid;
      int               request;
      int               status;
      std::string       header;
      std::string       data;
   };

   struct update
   {
      std::uint32_t     muid;
      std::string       id;
      std::string       command;
      std::string       data;
   };

   struct chain
   {
      using responder_type = ci::responder<fixed_random>;
      using properties_type =
         ci::property_responder<my_device&, responder_type&>;

      template <typename Send>
      void operator()(midi::sysex_view msg, Send&& send)
      {
         _properties(msg, send);
      }

      std::uint32_t muid() const { return _properties.muid(); }

      template <typename Send>
      void announce(Send&& send) { _properties.announce(send); }

      void property_reply(
         std::uint32_t muid, int request, int status
       , std::string_view header, q::byte_span data)
      {
         _replies.push_back(
            {muid, request, status, std::string{header}, text(data)});
      }

      void property_update(
         std::uint32_t muid, std::string_view id
       , std::string_view command, q::byte_span data)
      {
         _updates.push_back(
            {muid, std::string{id}, std::string{command}, text(data)});
      }

      void property_capabilities(std::uint32_t muid, int requests)
      {
         _capabilities.push_back({muid, requests});
      }

      my_device                        _device;
      responder_type                   _responder{me, fixed_random{}};
      properties_type                  _properties{_device, _responder};
      std::vector<reply>               _replies;
      std::vector<update>              _updates;
      std::vector<std::pair<std::uint32_t, int>> _capabilities;
   };

   template <std::size_t Capacity = 4096, std::size_t MaxRequests = 4>
   struct fixture
   {
      void receive(bytes const& b)
      {
         _initiator(midi::sysex_view{q::byte_span{b}}, _send);
      }

      q::byte_span sent(std::size_t i) const
      {
         auto const& b = _send._sent[i];
         return {b.data()+1, b.size()-2};
      }

      chain                                  _chain;
      ci::property_initiator<chain&, Capacity, MaxRequests>
                                             _initiator{_chain, 1000};
      sink                                   _send;
   };

   // A message from the synth to us, as the sysex readers deliver it.
   template <typename Make>
   bytes from_synth(Make make)
   {
      std::uint8_t out[1024];
      auto const n = make(out);
      return {out+1, out+n-1};
   }

   q::byte_span span(std::string_view s)
   {
      return {reinterpret_cast<std::uint8_t const*>(s.data()), s.size()};
   }
}

TEST_CASE("8.5 capabilities asks a device how many requests it takes")
{
   fixture f;
   f._initiator.capabilities(synth, f._send);

   ci::message_view const m{f.sent(0)};
   CHECK(m.sub_id() == ci::pe_status::capabilities);
   CHECK(m.destination() == synth.muid);

   f.receive(from_synth([](std::uint8_t* out)
   {
      return ci::make_pe_capabilities(
         out, synth.muid, 0x1234567, 3, true);
   }));
   REQUIRE(f._chain._capabilities.size() == 1);
   CHECK(f._chain._capabilities[0].first == synth.muid);
   CHECK(f._chain._capabilities[0].second == 3);
}

TEST_CASE("8.8 get asks for a resource by name and returns the request id")
{
   fixture f;
   auto const request = f._initiator.get(synth, "DeviceInfo", f._send);
   REQUIRE(request >= 0);

   ci::pe_view const v{f.sent(0)};
   REQUIRE(v.valid());
   CHECK(v.sub_id() == ci::pe_status::get);
   CHECK(v.destination() == synth.muid);
   CHECK(v.request_id() == request);
   CHECK(ci::header_value(v.header(), ci::pe_key::resource) == "DeviceInfo");
}

TEST_CASE("8.9 A reply to a Get reaches the application")
{
   fixture f;
   auto const request = f._initiator.get(synth, "DeviceInfo", f._send);

   f.receive(from_synth([&](std::uint8_t* out)
   {
      return ci::make_pe_get_reply(
         out, synth.muid, 0x1234567, std::uint8_t(request)
       , R"({"status":200})", span(R"({"model":"X"})"));
   }));

   REQUIRE(f._chain._replies.size() == 1);
   auto const& r = f._chain._replies[0];
   CHECK(r.muid == synth.muid);
   CHECK(r.request == request);
   CHECK(r.status == 200);
   CHECK(r.data == R"({"model":"X"})");
}

TEST_CASE("8.3 A reply in chunks is gathered before it is handed over")
{
   fixture f;
   auto const request = std::uint8_t(f._initiator.get(synth, "X", f._send));

   auto chunk = [&](std::uint16_t n, std::string_view header
    , std::string_view data)
   {
      return from_synth([&](std::uint8_t* out)
      {
         return ci::make_pe_get_reply(
            out, synth.muid, 0x1234567, request, header, span(data), 3, n);
      });
   };
   f.receive(chunk(1, R"({"status":200})", "abc"));
   f.receive(chunk(2, {}, "def"));
   CHECK(f._chain._replies.empty());
   f.receive(chunk(3, {}, "g"));

   REQUIRE(f._chain._replies.size() == 1);
   CHECK(f._chain._replies[0].status == 200);
   CHECK(f._chain._replies[0].data == "abcdefg");
}

TEST_CASE("8.3 A reply too long to gather is reported as 413")
{
   fixture<4> f;
   auto const request = std::uint8_t(f._initiator.get(synth, "X", f._send));

   f.receive(from_synth([&](std::uint8_t* out)
   {
      return ci::make_pe_get_reply(
         out, synth.muid, 0x1234567, request, R"({"status":200})"
       , span("too long"));
   }));

   REQUIRE(f._chain._replies.size() == 1);
   CHECK(f._chain._replies[0].status == ci::pe_reply::too_large);
   CHECK(f._chain._replies[0].data.empty());
}

TEST_CASE("8.10 set sends data in chunks no larger than the device takes")
{
   fixture f;
   auto small = synth;
   small.max_sysex_size = 128;
   std::string const data(300, 'x');

   auto const request = f._initiator.set(small, "X-Big", span(data), f._send);
   REQUIRE(request >= 0);
   REQUIRE(f._send._sent.size() > 2);

   std::string gathered;
   for (std::size_t i = 0; i != f._send._sent.size(); ++i)
   {
      CHECK(f._send._sent[i].size() <= 128);
      ci::pe_view const v{f.sent(i)};
      REQUIRE(v.valid());
      CHECK(v.sub_id() == ci::pe_status::set);
      CHECK(v.request_id() == request);
      CHECK(v.chunk() == i + 1);
      CHECK(v.chunks() == f._send._sent.size());
      if (i == 0)
         CHECK(ci::header_value(v.header(), ci::pe_key::resource) == "X-Big");
      else
         CHECK(v.header().empty());
      gathered += text(v.data());
   }
   CHECK(gathered == data);
}

TEST_CASE("8.11 The reply to a Set reaches the application")
{
   fixture f;
   auto const request = f._initiator.set(synth, "X-Gain", span("0.5"), f._send);

   f.receive(from_synth([&](std::uint8_t* out)
   {
      return ci::make_pe_set_reply(
         out, synth.muid, 0x1234567, std::uint8_t(request)
       , R"({"status":405})");
   }));

   REQUIRE(f._chain._replies.size() == 1);
   CHECK(f._chain._replies[0].request == request);
   CHECK(f._chain._replies[0].status == 405);
}

TEST_CASE("11.1 subscribe starts a subscription and the reply names it")
{
   fixture f;
   auto const request = f._initiator.subscribe(synth, "X-Gain", f._send);

   ci::pe_view const v{f.sent(0)};
   REQUIRE(v.valid());
   CHECK(v.sub_id() == ci::pe_status::subscription);
   CHECK(ci::header_value(v.header(), ci::pe_key::command) == "start");
   CHECK(ci::header_value(v.header(), ci::pe_key::resource) == "X-Gain");

   f.receive(from_synth([&](std::uint8_t* out)
   {
      return ci::make_pe_subscription_reply(
         out, synth.muid, 0x1234567, std::uint8_t(request)
       , R"({"status":200,"subscribeId":"g1"})");
   }));

   REQUIRE(f._chain._replies.size() == 1);
   CHECK(f._chain._replies[0].status == 200);
   CHECK(ci::header_value(f._chain._replies[0].header, ci::pe_key::subscribe_id)
      == "g1");
}

TEST_CASE("11 An update for our subscription is handed over and answered")
{
   fixture f;
   auto const request = f._initiator.subscribe(synth, "X-Gain", f._send);
   f.receive(from_synth([&](std::uint8_t* out)
   {
      return ci::make_pe_subscription_reply(
         out, synth.muid, 0x1234567, std::uint8_t(request)
       , R"({"status":200,"subscribeId":"g1"})");
   }));

   f.receive(from_synth([&](std::uint8_t* out)
   {
      return ci::make_pe_subscription(
         out, synth.muid, 0x1234567, 9
       , R"({"command":"full","subscribeId":"g1"})", span("0.25"));
   }));

   REQUIRE(f._chain._updates.size() == 1);
   CHECK(f._chain._updates[0].id == "g1");
   CHECK(f._chain._updates[0].command == "full");
   CHECK(f._chain._updates[0].data == "0.25");

   // 11: the update is answered with a status.
   ci::pe_view const ack{f.sent(f._send._sent.size() - 1)};
   REQUIRE(ack.valid());
   CHECK(ack.sub_id() == ci::pe_status::subscription_reply);
   CHECK(ack.request_id() == 9);
   CHECK(ack.destination() == synth.muid);
   CHECK(ack.header() == R"({"status":200})");
}

TEST_CASE("11.5 unsubscribe ends a subscription by its id")
{
   fixture f;
   auto const request = f._initiator.unsubscribe(synth, "g1", f._send);
   REQUIRE(request >= 0);

   ci::pe_view const v{f.sent(0)};
   REQUIRE(v.valid());
   CHECK(ci::header_value(v.header(), ci::pe_key::command) == "end");
   CHECK(ci::header_value(v.header(), ci::pe_key::subscribe_id) == "g1");
}

TEST_CASE("11 A subscription start from another device is the responder's")
{
   // Someone wants to subscribe to us: that is not an update for us.
   fixture f;
   f.receive(from_synth([&](std::uint8_t* out)
   {
      return ci::make_pe_subscription(
         out, synth.muid, 0x1234567, 4
       , R"({"command":"start","resource":"State"})");
   }));

   CHECK(f._chain._updates.empty());
   REQUIRE(f._send._sent.size() == 1);
   ci::pe_view const r{f.sent(0)};
   CHECK(r.sub_id() == ci::pe_status::subscription_reply);
   CHECK(r.header() == R"({"status":405})");
}

TEST_CASE("The requests in flight are bounded")
{
   fixture<4096, 2> f;
   CHECK(f._initiator.get(synth, "A", f._send) >= 0);
   CHECK(f._initiator.get(synth, "B", f._send) >= 0);
   CHECK(f._initiator.get(synth, "C", f._send) == -1);
   CHECK(f._send._sent.size() == 2);
}

TEST_CASE("A request with no reply expires after the window")
{
   fixture f;
   f._initiator.poll(500);
   auto const request = f._initiator.get(synth, "A", f._send);
   f._initiator.poll(1499);
   CHECK(f._chain._replies.empty());

   f._initiator.poll(1500);
   REQUIRE(f._chain._replies.size() == 1);
   CHECK(f._chain._replies[0].request == request);
   CHECK(f._chain._replies[0].status == ci::pe_reply::timed_out);

   // Its slot is free again.
   CHECK(f._initiator.get(synth, "B", f._send) >= 0);
}

TEST_CASE("A reply nobody here asked for is left alone")
{
   fixture f;
   f.receive(from_synth([&](std::uint8_t* out)
   {
      return ci::make_pe_get_reply(
         out, synth.muid, 0x1234567, 77, R"({"status":200})", span("x"));
   }));
   CHECK(f._chain._replies.empty());
}

#include <q/midi/profiles.hpp>

namespace
{
   // One application stage at the end of the initiator chain, in front of
   // the responders, hears every initiator's hooks.
   struct app
   {
      using responder_type = ci::responder<fixed_random>;

      template <typename Send>
      void operator()(midi::sysex_view msg, Send&& send)
      {
         _responder(msg, send);
      }

      std::uint32_t muid() const { return _responder.muid(); }

      template <typename Send>
      void announce(Send&& send) { _responder.announce(send); }

      void device_added(ci::remote_device const& d) { _heard.push_back(d.muid); }
      void property_reply(
         std::uint32_t, int, int status, std::string_view, q::byte_span)
      {
         _statuses.push_back(status);
      }
      void profiles_listed(std::uint32_t muid) { _listed.push_back(muid); }

      responder_type                _responder{me, fixed_random{}};
      std::vector<std::uint32_t>    _heard;
      std::vector<int>              _statuses;
      std::vector<std::uint32_t>    _listed;
   };
}

TEST_CASE("Initiators pass on the hooks they do not answer themselves")
{
   app a;
   ci::profile_initiator profiles{a};
   ci::property_initiator properties{profiles, 1000};
   ci::initiator discovery{properties, 1000};
   sink send;

   auto feed = [&](bytes const& b)
   {
      discovery(midi::sysex_view{q::byte_span{b}}, send);
   };

   discovery.poll(send, 0);
   feed(from_synth([](std::uint8_t* out)
   {
      return ci::make_discovery_reply(
         out, synth.muid, 0x1234567, synth.identity, 0x0C, 512, 0, 0x7F);
   }));
   CHECK(a._heard == std::vector<std::uint32_t>{synth.muid});

   auto const request = properties.get(synth, "DeviceInfo", send);
   feed(from_synth([&](std::uint8_t* out)
   {
      return ci::make_pe_get_reply(
         out, synth.muid, 0x1234567, std::uint8_t(request)
       , R"({"status":200})", span("{}"));
   }));
   CHECK(a._statuses == std::vector<int>{200});

   profiles.ask(synth, send);
   feed(from_synth([](std::uint8_t* out)
   {
      ci::profile const none[1] = {};
      return ci::make_profile_inquiry_reply(
         out, synth.muid, 0x1234567, ci::to_function_block
       , std::span<ci::profile const>{none, 0});
   }));
   CHECK(a._listed == std::vector<std::uint32_t>{synth.muid});
}

TEST_CASE("5.11 A NAK for our request ends it as refused")
{
   fixture f;
   auto const request = f._initiator.get(synth, "ResourceList", f._send);
   f.receive(from_synth([&](std::uint8_t* out)
   {
      return ci::make_pe_nak(
         out, synth.muid, 0x1234567, ci::pe_status::get
       , std::uint8_t(request), 0, ci::nak_status::not_supported);
   }));

   REQUIRE(f._chain._replies.size() == 1);
   CHECK(f._chain._replies[0].request == request);
   CHECK(f._chain._replies[0].status == ci::pe_reply::refused);
}

TEST_CASE("5.11 A NAK with no request id ends that device's oldest request")
{
   // Some devices leave the details empty.
   fixture f;
   f._initiator.get(synth, "A", f._send);               // request 0
   auto const second = f._initiator.get(synth, "B", f._send);
   auto const third = f._initiator.set(synth, "C", span("1"), f._send);
   f.receive(from_synth([&](std::uint8_t* out)
   {
      return ci::make_pe_nak(
         out, synth.muid, 0x1234567, ci::pe_status::set, 99, 0
       , ci::nak_status::not_supported);
   }));

   REQUIRE(f._chain._replies.size() == 1);
   CHECK(f._chain._replies[0].request == third);
   CHECK(f._chain._replies[0].status == ci::pe_reply::refused);
   CHECK(second != third);
}
