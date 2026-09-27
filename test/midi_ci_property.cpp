/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// MIDI-CI Property Exchange. M2-101-UM version 1.2 section 8 and Tables
// 30 to 39 for the messages, and M2-103-UM Common Rules for MIDI-CI
// Property Exchange version 1.2 for the header JSON, the status codes and
// the chunking.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/property_exchange.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <algorithm>
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

   void put14(bytes& b, std::uint16_t v)
   {
      b.push_back(v & 0x7F);
      b.push_back((v >> 7) & 0x7F);
   }

   // Table 33: a Get, written by hand. Request id first, then the header
   // and its length, then the two counters and an empty data field.
   bytes get_message(
      std::uint32_t source, std::uint32_t destination
    , std::uint8_t request, std::string_view header)
   {
      bytes b{0x7E, 0x7F, 0x0D, ci::pe_status::get, 0x02};
      put_muid(b, source);
      put_muid(b, destination);
      b.push_back(request);
      put14(b, std::uint16_t(header.size()));
      for (auto c : header)
         b.push_back(std::uint8_t(c));
      put14(b, 1);
      put14(b, 1);
      put14(b, 0);
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

   q::byte_span body(bytes const& b)
   {
      return q::byte_span{b.data()+1, b.size()-2};
   }

   std::string text(q::byte_span b)
   {
      return std::string{b.begin(), b.end()};
   }

   ci::identity const me{0x002109, 0x0102, 0x0304, 0x01020304};

   std::string_view const resources =
      R"([{"resource":"DeviceInfo"},{"resource":"ChannelList"}])";
   std::string_view const device_info =
      R"({"manufacturerId":[0,33,9],"familyId":[2,2]})";

   // What this device has. 9.1 makes only the first mandatory.
   struct device
   {
      std::string_view property(std::string_view name) const
      {
         if (name == ci::resource_list)
            return resources;
         if (name == "DeviceInfo")
            return device_info;
         return {};
      }

      int set_property(
         std::string_view name, q::byte_span data)
      {
         if (name != "State")
            return ci::pe_reply::not_found;
         _state.assign(data.begin(), data.end());
         return ci::pe_reply::ok;
      }

      bytes _state;
   };

   // One that only answers, to show what a Set gets from it.
   struct read_only
   {
      std::string_view property(std::string_view name) const
      {
         return name == ci::resource_list? resources : std::string_view{};
      }
   };

   template <typename Device = device
           , std::size_t Capacity = ci::default_max_sysex_size>
   struct fixture
   {
      using ci_responder = ci::responder<std::uint32_t(*)()>;

      fixture()
       : _ci{me, [] { return std::uint32_t(0x1234567); }}
       , _responder{_device, _ci}
      {}

      void receive(bytes const& b)
      {
         _responder(
            midi::sysex_view{q::byte_span{b}}, _send);
      }

      Device                              _device;
      ci_responder                        _ci;
      ci::property_responder<Device&, ci_responder&, Capacity>
                                          _responder;
      sink                                _send;
   };
}

TEST_CASE("7.1.1 A header value is read without a JSON parser")
{
   // 7.1.1 holds a header to one line with no whitespace, which is what
   // makes this safe: "The Header Data shall not include any whitespace
   // characters such as space, tab, line feed, or newline."
   CHECK(ci::header_value(R"({"resource":"DeviceInfo"})", "resource")
      == "DeviceInfo");
   CHECK(ci::header_value(
      R"({"resource":"X-Thing","resId":"2"})", "resId") == "2");
   CHECK(ci::header_value(R"({"status":200})", "resource").empty());
   CHECK(ci::header_value(R"({"resource":"A"})", "resId").empty());
}

TEST_CASE("Table 32 A device says how many requests it can have at once")
{
   fixture f;

   bytes b{0x7E, 0x7F, 0x0D, ci::pe_status::capabilities, 0x02};
   put_muid(b, 0x0ABCDEF);
   put_muid(b, 0x1234567);
   b.push_back(0x04);
   b.push_back(0x00);
   b.push_back(0x00);
   f.receive(b);

   REQUIRE(f._send._sent.size() == 1);
   ci::pe_capabilities_view const r{body(f._send._sent.front())};

   REQUIRE(r.valid());
   CHECK(r.sub_id() == ci::pe_status::capabilities_reply);
   CHECK(r.requests() == 1);

   // Version 1.2 added the two version bytes after the count.
   CHECK(r.major() == 0);
   CHECK(r.minor() == 0);
}

TEST_CASE("8.5 A version 1.1 capabilities message has the count alone")
{
   bytes b{0x7E, 0x7F, 0x0D, ci::pe_status::capabilities_reply, 0x01};
   put_muid(b, 0x0ABCDEF);
   put_muid(b, 0x1234567);
   b.push_back(0x02);

   ci::pe_capabilities_view const v{q::byte_span{b}};
   REQUIRE(v.valid());
   CHECK(v.requests() == 2);
   CHECK(v.major() == 0);
   CHECK(v.minor() == 0);
}

TEST_CASE("9.1 A device answers the resource list it is obliged to answer")
{
   // "All Devices that support Property Exchange shall respond correctly
   // to a ResourceList inquiry."
   fixture f;
   f.receive(
      get_message(0x0ABCDEF, 0x1234567, 7, R"({"resource":"ResourceList"})"));

   REQUIRE(f._send._sent.size() == 1);
   ci::pe_view const r{body(f._send._sent.front())};

   REQUIRE(r.valid());
   CHECK(r.sub_id() == ci::pe_status::get_reply);
   CHECK(r.request_id() == 7);
   CHECK(r.header() == R"({"status":200})");
   CHECK(r.chunks() == 1);
   CHECK(r.chunk() == 1);
   CHECK(r.last());
   CHECK(r.good());
   CHECK(text(r.data()) == resources);
}

TEST_CASE("8.4 A reply carries the request id the inquiry sent")
{
   fixture f;
   f.receive(
      get_message(0x0ABCDEF, 0x1234567, 99, R"({"resource":"DeviceInfo"})"));

   ci::pe_view const r{body(f._send._sent.front())};
   CHECK(r.request_id() == 99);
   CHECK(text(r.data()) == device_info);
}

TEST_CASE("7.4.1 A resource a device does not have is a 404")
{
   fixture f;
   f.receive(
      get_message(0x0ABCDEF, 0x1234567, 1, R"({"resource":"Nonesuch"})"));

   REQUIRE(f._send._sent.size() == 1);
   ci::pe_view const r{body(f._send._sent.front())};

   REQUIRE(r.valid());
   CHECK(r.header() == R"({"status":404})");
   CHECK(r.length() == 0);
}

TEST_CASE("8.3 A body too long for one message is sent in chunks")
{
   // A small buffer, so even a short answer has to be split.
   fixture<device, 64> f;
   f.receive(
      get_message(0x0ABCDEF, 0x1234567, 3, R"({"resource":"ResourceList"})"));

   REQUIRE(f._send._sent.size() > 1);

   std::string whole;
   for (std::size_t i = 0; i != f._send._sent.size(); ++i)
   {
      ci::pe_view const r{body(f._send._sent[i])};
      REQUIRE(r.valid());

      // 8.3: chunks count from one, and every one carries the request id.
      CHECK(r.chunk() == i + 1);
      CHECK(r.chunks() == f._send._sent.size());
      CHECK(r.request_id() == 3);

      // 8.3.1: "Header Data shall always be in the first Chunk only."
      if (i == 0)
         CHECK(r.header() == R"({"status":200})");
      else
         CHECK(r.header_length() == 0);

      whole += text(r.data());
   }

   CHECK(whole == resources);

   ci::pe_view const last{body(f._send._sent.back())};
   CHECK(last.last());
   CHECK(last.good());
}

TEST_CASE("8.3 A chunk count of zero means the sender does not know yet")
{
   std::array<std::uint8_t, 128> out = {};
   std::uint8_t const data[] = {1, 2, 3};

   auto const n = ci::make_pe_get_reply(
      out.data(), 0x1234567, 0x0ABCDEF, 1, R"({"status":200})"
    , q::byte_span{data}, 0, 1);

   ci::pe_view const v{q::byte_span{out.data()+1, n-2}};
   REQUIRE(v.valid());
   CHECK(v.chunks() == 0);
   CHECK(v.chunk() == 1);
   CHECK(!v.last());
   CHECK(v.good());
}

TEST_CASE("8.3 A last chunk numbered zero says the data is not to be trusted")
{
   // "If the sender does not know if all the Property Data sent is
   // complete or usable, then the Number of This Chunk for the final
   // Chunk shall be set to 0x0000."
   std::array<std::uint8_t, 128> out = {};

   auto const n = ci::make_pe_get_reply(
      out.data(), 0x1234567, 0x0ABCDEF, 1, "", {}, 6, 0);

   ci::pe_view const v{q::byte_span{out.data()+1, n-2}};
   REQUIRE(v.valid());
   CHECK(v.last());
   CHECK(!v.good());
}

TEST_CASE("Table 36 A set is confirmed with a status and no data")
{
   // 7.4: "the Responder shall confirm with a value of 200".
   std::array<std::uint8_t, 128> out = {};
   auto const n = ci::make_pe_set_reply(
      out.data(), 0x1234567, 0x0ABCDEF, 5, R"({"status":200})");

   ci::pe_view const v{q::byte_span{out.data()+1, n-2}};
   REQUIRE(v.valid());
   CHECK(v.sub_id() == ci::pe_status::set_reply);
   CHECK(v.request_id() == 5);
   CHECK(v.chunks() == 1);
   CHECK(v.chunk() == 1);
   CHECK(v.length() == 0);
}

TEST_CASE("Table 33 The request id comes first, before the header")
{
   // The 2020 editions of the two documents disagreed about this, and
   // version 1.2 settles it: the request id is the first payload byte.
   std::array<std::uint8_t, 128> out = {};
   auto const n = ci::make_pe_get(
      out.data(), 0x1234567, 0x0ABCDEF, 0x42, R"({"resource":"A"})");

   // 0xF0, then the thirteen byte header, so the payload starts at 14.
   CHECK(out[14] == 0x42);

   // Then the header length, fourteen bits least significant byte first.
   CHECK(out[15] == 16);
   CHECK(out[16] == 0);
   CHECK(out[17] == '{');
   CHECK(n > 14);
}

TEST_CASE("Anything that is not a property message goes to the next responder")
{
   fixture f;

   bytes b{0x7E, 0x7F, 0x0D, 0x70, 0x02};
   put_muid(b, 0x0ABCDEF);
   put_muid(b, ci::broadcast_muid);
   b.insert(b.end(), {0x00, 0x21, 0x09});
   b.insert(b.end(), {0x02, 0x02, 0x04, 0x06});
   b.insert(b.end(), {0x01, 0x02, 0x03, 0x04});
   b.push_back(0x00);
   b.insert(b.end(), {0x00, 0x04, 0x00, 0x00});
   b.push_back(0x00);
   f.receive(b);

   REQUIRE(f._send._sent.size() == 1);
   ci::discovery_reply_view const r{body(f._send._sent.front())};
   CHECK(r.valid());
}

namespace
{
   // Table 35: a Set, written by hand.
   bytes set_message(
      std::uint32_t source, std::uint32_t destination
    , std::uint8_t request, std::string_view header, bytes const& data)
   {
      bytes b{0x7E, 0x7F, 0x0D, ci::pe_status::set, 0x02};
      put_muid(b, source);
      put_muid(b, destination);
      b.push_back(request);
      put14(b, std::uint16_t(header.size()));
      for (auto c : header)
         b.push_back(std::uint8_t(c));
      put14(b, 1);
      put14(b, 1);
      put14(b, std::uint16_t(data.size()));
      b.insert(b.end(), data.begin(), data.end());
      return b;
   }
}

TEST_CASE("7.4 A set a device takes is confirmed with 200")
{
   fixture f;
   f.receive(
      set_message(
         0x0ABCDEF, 0x1234567, 11, R"({"resource":"State"})", {1, 2, 3}));

   REQUIRE(f._send._sent.size() == 1);
   ci::pe_view const r{body(f._send._sent.front())};

   REQUIRE(r.valid());
   CHECK(r.sub_id() == ci::pe_status::set_reply);
   CHECK(r.request_id() == 11);
   CHECK(r.header() == R"({"status":200})");
   CHECK(r.length() == 0);

   // The device is what took it.
   CHECK(f._device._state == bytes{1, 2, 3});
}

TEST_CASE("7.4.1 A set of a resource a device does not have is a 404")
{
   fixture f;
   f.receive(
      set_message(
         0x0ABCDEF, 0x1234567, 1, R"({"resource":"Nonesuch"})", {9}));

   ci::pe_view const r{body(f._send._sent.front())};
   CHECK(r.header() == R"({"status":404})");
}

TEST_CASE("7.4.1 A device that takes nothing refuses a set with 405")
{
   // It has no set_property at all, so the resource is not applicable.
   fixture<read_only> f;
   f.receive(
      set_message(
         0x0ABCDEF, 0x1234567, 1, R"({"resource":"State"})", {9}));

   REQUIRE(f._send._sent.size() == 1);
   ci::pe_view const r{body(f._send._sent.front())};
   CHECK(r.sub_id() == ci::pe_status::set_reply);
   CHECK(r.header() == R"({"status":405})");
}

TEST_CASE("6.1.7 Seven bytes of eight bit data become eight of seven")
{
   // "First, the sign bits of the seven bytes are sent, followed by the
   // low-order 7 bits of each byte."
   std::uint8_t const data[] = {0x80, 0x01, 0xFF, 0x00, 0x7F, 0x81, 0xC3};
   std::array<std::uint8_t, 16> out = {};

   auto const n = ci::mcoded7_encode(
      q::byte_span{data}, std::span<std::uint8_t>{out});

   REQUIRE(n == 8);
   CHECK(n == ci::mcoded7_size(7));

   // A, C, F and G have their top bit set: 0b1010011 = 0x53.
   CHECK(out[0] == 0x53);
   CHECK(out[1] == 0x00);
   CHECK(out[2] == 0x01);
   CHECK(out[3] == 0x7F);
   CHECK(out[4] == 0x00);
   CHECK(out[5] == 0x7F);
   CHECK(out[6] == 0x01);
   CHECK(out[7] == 0x43);
}

TEST_CASE("6.1.7 A last group shorter than seven keeps the sign bits high")
{
   // "AAAAaaaa BBBBbbbb CCCCcccc are transmitted as 0ABC0000 ...", so the
   // bits sit where they would in a full group.
   std::uint8_t const data[] = {0x80, 0x00, 0x80};
   std::array<std::uint8_t, 8> out = {};

   auto const n = ci::mcoded7_encode(
      q::byte_span{data}, std::span<std::uint8_t>{out});

   REQUIRE(n == 4);
   CHECK(out[0] == 0x50);           // A and C set: 0b1010000
   CHECK(out[1] == 0x00);
   CHECK(out[2] == 0x00);
   CHECK(out[3] == 0x00);
}

TEST_CASE("6.1.7 Everything that goes in comes back out")
{
   bytes data;
   for (int i = 0; i != 300; ++i)
      data.push_back(std::uint8_t(i * 7 + i / 3));

   bytes encoded(ci::mcoded7_size(data.size()));
   auto const n = ci::mcoded7_encode(data, encoded);
   REQUIRE(n == encoded.size());

   // Every encoded byte is seven bit, which is the point.
   for (auto b : encoded)
      CHECK(b < 0x80);

   bytes decoded(ci::mcoded7_decoded_size(encoded.size()));
   auto const m = ci::mcoded7_decode(encoded, decoded);

   REQUIRE(m == data.size());
   CHECK(decoded == data);
}

TEST_CASE("Table 94 A flow control acknowledgement names its chunk")
{
   std::array<std::uint8_t, 64> out = {};
   auto const n = ci::make_pe_ack(
      out.data(), 0x1234567, 0x0ABCDEF, ci::pe_status::get_reply, 5, 882);

   ci::ack_view const v{q::byte_span{out.data()+1, n-2}};

   REQUIRE(v.valid());
   CHECK(v.original_sub_id() == ci::pe_status::get_reply);
   CHECK(v.status() == ci::ack_status::send_next);
   CHECK(ci::pe_details_request(v.details()) == 5);
   CHECK(ci::pe_details_chunk(v.details()) == 882);
   CHECK(v.message_length() == 0);
}

TEST_CASE("Table 97 A retransmit names the last chunk that did arrive")
{
   std::array<std::uint8_t, 64> out = {};
   auto const n = ci::make_pe_nak(
      out.data(), 0x1234567, 0x0ABCDEF, ci::pe_status::get_reply, 5, 881);

   ci::nak_view const v{q::byte_span{out.data()+1, n-2}};

   REQUIRE(v.valid());
   CHECK(v.status() == ci::nak_status::resend_chunk);
   CHECK(ci::pe_details_request(v.details()) == 5);
   CHECK(ci::pe_details_chunk(v.details()) == 881);
}

TEST_CASE("11.1 A subscription leads with its command")
{
   std::array<std::uint8_t, 128> out = {};
   auto const n = ci::make_pe_subscription(
      out.data(), 0x1234567, 0x0ABCDEF, 2
    , R"({"command":"start","resource":"State"})");

   ci::pe_view const v{q::byte_span{out.data()+1, n-2}};

   REQUIRE(v.valid());
   CHECK(v.sub_id() == ci::pe_status::subscription);
   CHECK(ci::header_value(v.header(), ci::pe_key::command)
      == ci::pe_command::start);
   CHECK(ci::header_value(v.header(), ci::pe_key::resource) == "State");
}

TEST_CASE("7.4 A status header is written for any code")
{
   std::array<char, 24> buffer = {};

   CHECK(ci::status_header(buffer, ci::pe_reply::ok) == R"({"status":200})");
   CHECK(ci::status_header(buffer, ci::pe_reply::too_many_requests)
      == R"({"status":343})");
   CHECK(ci::status_header(buffer, ci::pe_reply::internal_error)
      == R"({"status":500})");
}

namespace
{
   // Table 37: a subscription, written by hand.
   bytes subscribe_message(
      std::uint32_t source, std::uint32_t destination
    , std::uint8_t request, std::string_view header)
   {
      bytes b{0x7E, 0x7F, 0x0D, ci::pe_status::subscription, 0x02};
      put_muid(b, source);
      put_muid(b, destination);
      b.push_back(request);
      put14(b, std::uint16_t(header.size()));
      for (auto c : header)
         b.push_back(std::uint8_t(c));
      put14(b, 1);
      put14(b, 1);
      put14(b, 0);
      return b;
   }

   bytes invalidate_message(std::uint32_t source, std::uint32_t target)
   {
      bytes b{0x7E, 0x7F, 0x0D, ci::sub_id::invalidate_muid, 0x02};
      put_muid(b, source);
      put_muid(b, ci::broadcast_muid);
      put_muid(b, target);
      return b;
   }

   // A device that keeps its own subscribers, which is where 11.5 puts
   // the table.
   struct subscriber_device
   {
      struct entry
      {
         std::string    id;
         std::uint32_t  muid;
         std::string    resource;
      };

      std::string_view property(std::string_view name) const
      {
         return name == ci::resource_list? resources : std::string_view{};
      }

      std::string_view subscribe(
         std::uint32_t muid, std::string_view resource)
      {
         if (resource != "State")
            return {};
         _held.push_back(
            {"sub" + std::to_string(++_next), muid, std::string{resource}});
         return _held.back().id;
      }

      void unsubscribe(std::string_view id)
      {
         std::erase_if(_held, [&](entry const& e) { return e.id == id; });
      }

      void muid_invalidated(std::uint32_t target, bool all)
      {
         if (all)
            _held.clear();
         else
            std::erase_if(
               _held, [&](entry const& e) { return e.muid == target; });
      }

      std::vector<entry> _held;
      int                _next = 0;
   };
}

TEST_CASE("11 A subscription is started and the device names it")
{
   fixture<subscriber_device> f;
   f.receive(
      subscribe_message(
         0x0ABCDEF, 0x1234567, 4
       , R"({"command":"start","resource":"State"})"));

   REQUIRE(f._send._sent.size() == 1);
   ci::pe_view const r{body(f._send._sent.front())};

   REQUIRE(r.valid());
   CHECK(r.sub_id() == ci::pe_status::subscription_reply);
   CHECK(r.request_id() == 4);
   CHECK(ci::header_value(r.header(), ci::pe_key::status) == "");
   CHECK(r.header() == R"({"status":200,"subscribeId":"sub1"})");

   // 11.5 puts the table on the device, and this is it.
   REQUIRE(f._device._held.size() == 1);
   CHECK(f._device._held[0].muid == 0x0ABCDEF);
   CHECK(f._device._held[0].resource == "State");
}

TEST_CASE("7.4.1 A resource the device will not subscribe is a 405")
{
   fixture<subscriber_device> f;
   f.receive(
      subscribe_message(
         0x0ABCDEF, 0x1234567, 1
       , R"({"command":"start","resource":"Nonesuch"})"));

   ci::pe_view const r{body(f._send._sent.front())};
   CHECK(r.header() == R"({"status":405})");
   CHECK(f._device._held.empty());
}

TEST_CASE("11.5 Either end may end a subscription")
{
   fixture<subscriber_device> f;
   f.receive(
      subscribe_message(
         0x0ABCDEF, 0x1234567, 1
       , R"({"command":"start","resource":"State"})"));
   REQUIRE(f._device._held.size() == 1);

   f._send._sent.clear();
   f.receive(
      subscribe_message(
         0x0ABCDEF, 0x1234567, 2
       , R"({"command":"end","subscribeId":"sub1"})"));

   REQUIRE(f._send._sent.size() == 1);
   ci::pe_view const r{body(f._send._sent.front())};
   CHECK(r.header() == R"({"status":200})");
   CHECK(f._device._held.empty());
}

TEST_CASE("11.5 An invalidated MUID ends the subscriptions held with it")
{
   fixture<subscriber_device> f;
   f.receive(
      subscribe_message(
         0x0ABCDEF, 0x1234567, 1
       , R"({"command":"start","resource":"State"})"));
   REQUIRE(f._device._held.size() == 1);

   // Someone else's MUID goes: ours are untouched.
   f.receive(invalidate_message(0x0ABCDEF, 0x0111111));
   CHECK(f._device._held.size() == 1);

   // Theirs goes: the subscription they held is over.
   f.receive(invalidate_message(0x0ABCDEF, 0x0ABCDEF));
   CHECK(f._device._held.empty());
}

TEST_CASE("11.5 Our own MUID going ends every subscription at once")
{
   fixture<subscriber_device> f;
   f.receive(
      subscribe_message(
         0x0ABCDEF, 0x1234567, 1
       , R"({"command":"start","resource":"State"})"));
   REQUIRE(f._device._held.size() == 1);

   f.receive(invalidate_message(0x0ABCDEF, 0x1234567));
   CHECK(f._device._held.empty());
}

TEST_CASE("11 An update carries the command the device chose")
{
   fixture<subscriber_device> f;
   std::uint8_t const data[] = {'{', '}'};

   f._responder.update(
      0x0ABCDEF, "sub1", ci::pe_command::full
    , q::byte_span{data}, f._send);

   REQUIRE(f._send._sent.size() == 1);
   ci::pe_view const r{body(f._send._sent.front())};

   REQUIRE(r.valid());
   CHECK(r.sub_id() == ci::pe_status::subscription);
   CHECK(r.destination() == 0x0ABCDEF);
   CHECK(ci::header_value(r.header(), ci::pe_key::command)
      == ci::pe_command::full);
   CHECK(ci::header_value(r.header(), ci::pe_key::subscribe_id) == "sub1");
   CHECK(text(r.data()) == "{}");
}

TEST_CASE("11 A device with no subscription hooks refuses a start")
{
   // The three hooks are optional, and one that has none is unaffected.
   fixture<read_only> f;
   f.receive(
      subscribe_message(
         0x0ABCDEF, 0x1234567, 1
       , R"({"command":"start","resource":"State"})"));

   REQUIRE(f._send._sent.size() == 1);
   ci::pe_view const r{body(f._send._sent.front())};
   CHECK(r.sub_id() == ci::pe_status::subscription_reply);
   CHECK(r.header() == R"({"status":405})");
}

////////////////////////////////////////////////////////////////////////////
// A Sink with a send member serves the property responder as a callable does.
////////////////////////////////////////////////////////////////////////////
namespace
{
   struct member_sink
   {
      void send(q::byte_span out) { _sent.push_back({out.begin(), out.end()}); }
      std::vector<bytes> _sent;
   };
}

TEST_CASE("A Sink with a send member serves as well as a callable")
{
   bytes b{0x7E, 0x7F, 0x0D, ci::pe_status::capabilities, 0x02};
   put_muid(b, 0x0ABCDEF);
   put_muid(b, 0x1234567);
   b.push_back(0x04);
   b.push_back(0x00);
   b.push_back(0x00);

   fixture f;
   f.receive(b);

   device d;
   ci::responder<std::uint32_t(*)()> discovery{
      me, [] { return std::uint32_t(0x1234567); }};
   ci::property_responder<device&, decltype(discovery)&> r{d, discovery};
   member_sink out;
   r(midi::sysex_view{q::byte_span{b}}, out);

   REQUIRE(out._sent.size() == 1);
   CHECK(out._sent.front() == f._send._sent.front());
}
