/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// MIDI-CI Profile Configuration. M2-101-UM version 1.2 section 7 for the
// messages and their tables, and M2-102-U Common Rules for MIDI-CI
// Profiles version 1.1 for what a device must reply in each case.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/profiles.hpp>
#include <q/midi/property_exchange.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <string>
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

   bytes message(
      std::uint8_t device, std::uint8_t sub, std::uint32_t source
    , std::uint32_t destination, bytes const& payload = {})
   {
      bytes b{0x7E, device, 0x0D, sub, 0x02};
      put_muid(b, source);
      put_muid(b, destination);
      b.insert(b.end(), payload.begin(), payload.end());
      return b;
   }

   bytes id_bytes(ci::profile_id const& id)
   {
      return {id.byte1, id.byte2, id.byte3, id.byte4, id.byte5};
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

   // A standard profile, and one of a manufacturer's own.
   ci::profile_id const organ{0x7E, 0x40, 0x01, 0x01, 0x01};
   ci::profile_id const mine{0x00, 0x21, 0x09, 0x01, 0x01};

   ci::identity const me{0x002109, 0x0102, 0x0304, 0x01020304};

   struct fixture
   {
      fixture(std::vector<ci::profile> profiles)
       : _profiles(std::move(profiles))
       , _ci{me, [] { return std::uint32_t(0x1234567); }}
       , _responder{std::span<ci::profile>{_profiles}, _ci}
      {}

      void receive(bytes const& b)
      {
         _responder(
            midi::sysex_view{q::byte_span{b}}, _send);
      }

      std::vector<ci::profile>            _profiles;
      using ci_responder = ci::responder<std::uint32_t(*)()>;

      ci_responder                        _ci;
      ci::profile_responder<ci_responder&>
                                          _responder;
      sink                                _send;
   };
}

TEST_CASE("Table 19 A standard profile is told from a manufacturer's own")
{
   // 7.3.1: "The value of the Profile ID Byte 1 is 0x7E (Universal)."
   CHECK(organ.standard());
   CHECK(!mine.standard());
}

TEST_CASE("3.2 Two profiles are the same when only the level differs")
{
   // "A Sender uses the value 0x7F for the Profile Level Support field
   // when sending a Set Profile On message", so an exact comparison would
   // never match what a device declared.
   ci::profile_id const asked{
      0x7E, 0x40, 0x01, 0x01, ci::profile_level::highest};

   CHECK(!(asked == organ));
   CHECK(ci::same_profile(asked, organ));
}

TEST_CASE("Table 18 An inquiry is answered with what is on and what is off")
{
   fixture f{{
      {organ, ci::to_function_block, true, false, 0, 0}
    , {mine, ci::to_function_block, false, false, 0, 0}}};

   f.receive(
      message(
         ci::to_function_block, ci::profile_status::inquiry
       , 0x0ABCDEF, 0x1234567));

   REQUIRE(f._send._sent.size() == 1);
   ci::profile_inquiry_reply_view const r{body(f._send._sent.front())};

   REQUIRE(r.valid());
   CHECK(r.enabled() == 1);
   CHECK(r.disabled() == 1);
   CHECK(r.enabled_profile(0) == organ);
   CHECK(r.disabled_profile(0) == mine);
}

TEST_CASE("2.4 A function block inquiry answers per address, its own last")
{
   // "As a final reply, the Responder shall send a Reply to Profile
   // Inquiry message with address 0x7F", which is what says no more are
   // coming.
   fixture f{{
      {organ, 0x02, true, false, 0, 0}          // on channel 3
    , {mine, ci::to_group, true, false, 0, 0}
    , {organ, ci::to_function_block, false, false, 0, 0}}};

   f.receive(
      message(
         ci::to_function_block, ci::profile_status::inquiry
       , 0x0ABCDEF, 0x1234567));

   REQUIRE(f._send._sent.size() == 3);

   std::vector<std::uint8_t> addresses;
   for (auto const& b : f._send._sent)
      addresses.push_back(b[2]);

   CHECK(addresses
      == std::vector<std::uint8_t>{0x02, ci::to_group, ci::to_function_block});
}

TEST_CASE("2.4 An inquiry with no profiles at all still gets the last reply")
{
   fixture f{{}};
   f.receive(
      message(
         ci::to_function_block, ci::profile_status::inquiry
       , 0x0ABCDEF, 0x1234567));

   REQUIRE(f._send._sent.size() == 1);
   ci::profile_inquiry_reply_view const r{body(f._send._sent.front())};
   CHECK(r.valid());
   CHECK(r.enabled() == 0);
   CHECK(r.disabled() == 0);
}

TEST_CASE("2.6 Turning a profile on reports that it is on")
{
   fixture f{{{organ, ci::to_function_block, false, false, 0, 4}}};

   auto payload = id_bytes(organ);
   payload.push_back(0x04);            // two channels asked for
   payload.push_back(0x00);
   f.receive(
      message(
         ci::to_function_block, ci::profile_status::set_on
       , 0x0ABCDEF, 0x1234567, payload));

   REQUIRE(f._send._sent.size() == 1);
   ci::profile_report_view const r{body(f._send._sent.front())};

   REQUIRE(r.valid());
   CHECK(r.enabled());
   CHECK(r.id() == organ);

   // 7.10: the report goes to everyone.
   CHECK(r.destination() == ci::broadcast_muid);

   // The caller's own list is what changed.
   CHECK(f._profiles[0].enabled);
}

TEST_CASE("2.7 Turning one off reports that it is off")
{
   fixture f{{{organ, ci::to_function_block, true, false, 2, 4}}};

   auto payload = id_bytes(organ);
   payload.push_back(0x00);
   payload.push_back(0x00);
   f.receive(
      message(
         ci::to_function_block, ci::profile_status::set_off
       , 0x0ABCDEF, 0x1234567, payload));

   REQUIRE(f._send._sent.size() == 1);
   ci::profile_report_view const r{body(f._send._sent.front())};
   REQUIRE(r.valid());
   CHECK(!r.enabled());
   CHECK(!f._profiles[0].enabled);
}

TEST_CASE("2.7 A profile that cannot be turned off says it is still on")
{
   // "if a device is not able to disable a particular Profile, then it
   // shall reply with a Profile Enabled message", which is 2.5's acoustic
   // piano that always conforms to the piano profile.
   fixture f{{{organ, ci::to_function_block, true, true, 2, 4}}};

   auto payload = id_bytes(organ);
   payload.push_back(0x00);
   payload.push_back(0x00);
   f.receive(
      message(
         ci::to_function_block, ci::profile_status::set_off
       , 0x0ABCDEF, 0x1234567, payload));

   REQUIRE(f._send._sent.size() == 1);
   ci::profile_report_view const r{body(f._send._sent.front())};
   REQUIRE(r.valid());
   CHECK(r.enabled());
   CHECK(f._profiles[0].enabled);
}

TEST_CASE("2.6 A profile we do not have gets a NAK with status 0x04")
{
   // Table 16: "Profile not supported on the requested Channel, Group, or
   // Function Block".
   fixture f{{{organ, ci::to_function_block, false, false, 0, 0}}};

   auto payload = id_bytes(mine);
   payload.push_back(0x00);
   payload.push_back(0x00);
   f.receive(
      message(
         ci::to_function_block, ci::profile_status::set_on
       , 0x0ABCDEF, 0x1234567, payload));

   REQUIRE(f._send._sent.size() == 1);
   ci::nak_view const n{body(f._send._sent.front())};
   REQUIRE(n.valid());
   CHECK(n.original_sub_id() == ci::profile_status::set_on);
   CHECK(n.status() == ci::nak_status::profile_not_supported);
}

TEST_CASE("Table 8 The details of a profile are its channels")
{
   fixture f{{{organ, ci::to_function_block, true, false, 2, 6}}};

   auto payload = id_bytes(organ);
   payload.push_back(ci::profile_target::channels);
   f.receive(
      message(
         ci::to_function_block, ci::profile_status::details
       , 0x0ABCDEF, 0x1234567, payload));

   REQUIRE(f._send._sent.size() == 1);
   ci::profile_details_reply_view const r{body(f._send._sent.front())};

   REQUIRE(r.valid());
   CHECK(r.id() == organ);
   CHECK(r.target() == ci::profile_target::channels);
   CHECK(r.length() == 4);
   CHECK(r.channels() == 2);
   CHECK(r.max_channels() == 6);
}

TEST_CASE("2.5.1 A profile that is off is using no channels")
{
   // "If the Profile is not currently enabled, set to 0x00 0x00."
   fixture f{{{organ, ci::to_function_block, false, false, 2, 6}}};

   auto payload = id_bytes(organ);
   payload.push_back(ci::profile_target::channels);
   f.receive(
      message(
         ci::to_function_block, ci::profile_status::details
       , 0x0ABCDEF, 0x1234567, payload));

   ci::profile_details_reply_view const r{body(f._send._sent.front())};
   CHECK(r.channels() == 0);
   CHECK(r.max_channels() == 6);
}

TEST_CASE("Table 28 Profile specific data counts its length in four bytes")
{
   // Every other length in this category is two bytes; this one is four.
   std::array<std::uint8_t, 64> out = {};
   auto n = ci::detail::header_at(
      out.data(), ci::to_function_block, ci::profile_status::specific_data
    , 0x1234567, 0x0ABCDEF);
   n += ci::detail::write_profile(out.data()+n, organ);
   ci::write_muid(out.data()+n, 3);
   n += 4;
   out[n++] = 0x11;
   out[n++] = 0x22;
   out[n++] = 0x33;
   out[n++] = 0xF7;

   ci::profile_data_view const v{
      q::byte_span{out.data()+1, n-2}};

   REQUIRE(v.valid());
   CHECK(v.id() == organ);
   CHECK(v.length() == 3);
   CHECK(v.data().size() == 3);
   CHECK(v.data()[2] == 0x33);
}

TEST_CASE("7.4 A profile gained or lost is reported to everyone")
{
   std::array<std::uint8_t, 64> out = {};
   auto const n = ci::make_profile_list_report(
      out.data(), 0x1234567, ci::to_function_block, organ, true);

   ci::profile_list_view const v{
      q::byte_span{out.data()+1, n-2}};

   REQUIRE(v.valid());
   CHECK(v.added());
   CHECK(v.id() == organ);
   CHECK(v.destination() == ci::broadcast_muid);
}

TEST_CASE("Anything that is not a profile message goes to the next responder")
{
   fixture f{{}};

   // A discovery, which the MIDI-CI responder behind this one answers.
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

TEST_CASE("Responders nest, so one device can have profiles and properties")
{
   // A device with nothing to say about properties: every Get is a 404.
   struct silent_device
   {
      std::string_view property(std::string_view) const { return {}; }
   };

   std::vector<ci::profile> profiles{
      {organ, ci::to_function_block, true, false, 0, 0}};
   ci::responder discovery{me, [] { return std::uint32_t(0x1234567); }};
   ci::profile_responder in_front{std::span<ci::profile>{profiles}, discovery};
   silent_device device;
   ci::property_responder outermost{device, in_front};
   sink send;

   // The MUID is the discovery responder's, seen through both layers.
   CHECK(outermost.muid() == 0x1234567);

   // A profile inquiry given to the outermost responder passes through the
   // property one and is answered by the profile one.
   outermost(
      midi::sysex_view{q::byte_span{
         message(ci::to_function_block, ci::profile_status::inquiry
               , 0x0ABCDEF, 0x1234567)}}
    , send);

   REQUIRE(send._sent.size() == 1);
   ci::profile_inquiry_reply_view const r{body(send._sent.front())};
   REQUIRE(r.valid());
   CHECK(r.enabled() == 1);
   CHECK(r.enabled_profile(0) == organ);
}

////////////////////////////////////////////////////////////////////////////
// A Sink with a send member serves the profile responder as a callable does.
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
   std::vector<ci::profile> profiles{
      {organ, ci::to_function_block, true, false, 0, 0}
    , {mine, ci::to_function_block, false, false, 0, 0}};
   fixture f{profiles};
   auto const b = message(
      ci::to_function_block, ci::profile_status::inquiry, 0x0ABCDEF, 0x1234567);
   f.receive(b);

   ci::responder<std::uint32_t(*)()> discovery{
      me, [] { return std::uint32_t(0x1234567); }};
   ci::profile_responder<decltype(discovery)&> r{
      std::span<ci::profile>{profiles}, discovery};
   member_sink out;
   r(midi::sysex_view{q::byte_span{b}}, out);

   REQUIRE(out._sent.size() == 1);
   CHECK(out._sent.front() == f._send._sent.front());
}
