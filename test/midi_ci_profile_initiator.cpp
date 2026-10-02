/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The initiator's side of profile configuration. From M2-102-U Common
// Rules for MIDI-CI Profiles version 1.1: sections 7.2, 7.3, 7.8 to 7.11,
// Tables 17 to 19 and 24 to 27. Cases are named for their clause.

#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/profiles.hpp>

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

   struct fixed_random
   {
      std::uint32_t operator()() const { return 0x1234567; }
   };

   ci::identity const me{0x002109, 0x0102, 0x0304, 0x01020304};
   ci::profile_id const organ{0x7E, 0x40, 0x01, 0x01, 0x01};
   ci::profile_id const mine{0x00, 0x21, 0x09, 0x01, 0x01};

   ci::remote_device const synth{
      0x0AAAAAA, {0x7D0000, 1, 1, 0}, 0x0C, 512, 0, 0x7F};

   struct state
   {
      std::uint32_t     muid;
      std::uint8_t      address;
      ci::profile_id    id;
      bool              enabled;
   };

   // The stage after the initiator: a responder, and the hooks an
   // application provides.
   struct chain
   {
      template <typename Send>
      void operator()(midi::sysex_view msg, Send&& send)
      {
         _responder(msg, send);
      }

      std::uint32_t muid() const { return _responder.muid(); }

      template <typename Send>
      void announce(Send&& send) { _responder.announce(send); }

      void profile_state(
         std::uint32_t muid, std::uint8_t address
       , ci::profile_id const& id, bool enabled)
      {
         _states.push_back({muid, address, id, enabled});
      }

      void profiles_listed(std::uint32_t muid)
      {
         _listed.push_back(muid);
      }

      ci::responder<fixed_random>   _responder{me, fixed_random{}};
      std::vector<state>            _states;
      std::vector<std::uint32_t>    _listed;
   };

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

      chain                            _chain;
      ci::profile_initiator<chain&>    _initiator{_chain};
      sink                             _send;
   };

   template <typename Profiles>
   bytes inquiry_reply(std::uint8_t address, Profiles const& profiles)
   {
      std::uint8_t out[256];
      auto const n = ci::make_profile_inquiry_reply(
         out, synth.muid, 0x1234567, address, profiles);
      return {out+1, out+n-1};
   }
}

TEST_CASE("7.2 ask sends a Profile Inquiry to a device's function block")
{
   fixture f;
   f._initiator.ask(synth, f._send);

   REQUIRE(f._send._sent.size() == 1);
   ci::message_view const m{f.sent(0)};
   REQUIRE(m.has_header());
   CHECK(m.sub_id() == ci::profile_status::inquiry);
   CHECK(m.device_id() == ci::to_function_block);
   CHECK(m.source() == 0x1234567);
   CHECK(m.destination() == synth.muid);
}

TEST_CASE("7.3 Each profile in a reply reaches the application")
{
   // The function block's reply comes last, so it ends the listing.
   fixture f;
   f._initiator.ask(synth, f._send);

   ci::profile const channel[] = {{mine, 0, true, false, 1, 1}};
   ci::profile const block[] = {
      {organ, ci::to_function_block, false}
    , {mine, ci::to_function_block, true}};
   f.receive(inquiry_reply(0, channel));
   CHECK(f._chain._listed.empty());
   f.receive(inquiry_reply(ci::to_function_block, block));

   REQUIRE(f._chain._states.size() == 3);
   CHECK(f._chain._states[0].muid == synth.muid);
   CHECK(f._chain._states[0].address == 0);
   CHECK(f._chain._states[0].id == mine);
   CHECK(f._chain._states[0].enabled);
   CHECK(f._chain._states[1].address == ci::to_function_block);
   CHECK(f._chain._states[1].id == mine);
   CHECK(f._chain._states[1].enabled);
   CHECK(f._chain._states[2].id == organ);
   CHECK(!f._chain._states[2].enabled);
   CHECK(f._chain._listed == std::vector<std::uint32_t>{synth.muid});
}

TEST_CASE("7.8 turn_on asks for a profile at an address, with its channels")
{
   fixture f;
   f._initiator.turn_on(synth, 3, mine, f._send, 1);

   REQUIRE(f._send._sent.size() == 1);
   ci::set_profile_view const v{f.sent(0)};
   REQUIRE(v.valid());
   CHECK(v.on());
   CHECK(v.device_id() == 3);
   CHECK(v.id() == mine);
   CHECK(v.channels() == 1);
   CHECK(v.destination() == synth.muid);
}

TEST_CASE("7.9 turn_off asks for it to be turned off")
{
   fixture f;
   f._initiator.turn_off(synth, ci::to_function_block, organ, f._send);

   ci::set_profile_view const v{f.sent(0)};
   REQUIRE(v.valid());
   CHECK(!v.on());
   CHECK(v.id() == organ);
}

TEST_CASE("7.10 A device's report of a profile's state reaches us")
{
   // Reports go to everyone, whether or not we asked.
   fixture f;
   std::uint8_t out[64];
   auto const n = ci::make_profile_report(
      out, synth.muid, ci::to_function_block, organ, true);
   f.receive(bytes{out+1, out+n-1});

   REQUIRE(f._chain._states.size() == 1);
   CHECK(f._chain._states[0].muid == synth.muid);
   CHECK(f._chain._states[0].id == organ);
   CHECK(f._chain._states[0].enabled);
}

TEST_CASE("A reply addressed to another MUID is not ours")
{
   fixture f;
   std::uint8_t out[64];
   ci::profile const block[] = {{organ}};
   auto const n = ci::make_profile_inquiry_reply(
      out, synth.muid, 0x0123456, ci::to_function_block, block);
   f.receive(bytes{out+1, out+n-1});

   CHECK(f._chain._states.empty());
   CHECK(f._chain._listed.empty());
}

TEST_CASE("Our own reports, heard back, are not another device's")
{
   fixture f;
   std::uint8_t out[64];
   auto const n = ci::make_profile_report(
      out, 0x1234567, ci::to_function_block, organ, true);
   f.receive(bytes{out+1, out+n-1});

   CHECK(f._chain._states.empty());
}
