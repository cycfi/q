/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/controllers.hpp>
#include <q/midi/parameters.hpp>

#include <cstdint>
#include <functional>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;

namespace
{
   struct recorder : midi::processor
   {
      using midi::processor::operator();

      struct wide
      {
         std::uint8_t   channel;
         std::uint8_t   controller;
         std::uint16_t  value;
      };

      struct narrow
      {
         std::uint8_t   channel;
         std::uint8_t   controller;
         std::uint8_t   value;
      };

      void operator()(midi::control_change_14 msg, std::size_t)
      {
         _wide.push_back(
            {msg.channel(), std::uint8_t(msg.controller()), msg.value()});
      }

      void operator()(midi::control_change msg, std::size_t)
      {
         _narrow.push_back(
            {msg.channel(), std::uint8_t(msg.controller()), msg.value()});
      }

      void operator()(midi::note_on, std::size_t)
      {
         ++_notes;
      }

      struct param
      {
         std::uint16_t  number;
         std::uint16_t  value;
      };

      void operator()(midi::rpn msg, std::size_t)
      {
         _params.push_back({msg.number(), msg.value()});
      }

      std::vector<param>   _params;
      std::vector<wide>    _wide;
      std::vector<narrow>  _narrow;
      int                  _notes = 0;
   };

   // Only the bytes the message has: reading past its own size is out of
   // bounds, and an optimizer is entitled to act on that.
   template <typename Message>
   midi::raw_message to_raw(Message const& msg)
   {
      std::uint32_t data = 0;
      for (int i = 0; i != Message::size; ++i)
         data |= std::uint32_t(msg.data[i]) << (i * 8);
      return {data};
   }

   struct fixture
   {
      void cc(std::uint8_t channel, std::uint8_t ctrl, std::uint8_t value)
      {
         send(midi::control_change{channel, midi::cc::controller(ctrl), value});
      }

      template <typename Message>
      void send(Message const& msg)
      {
         midi::raw_message const raw = to_raw(msg);
         midi::dispatch(raw, _time++, _chain);
      }

      recorder                                  _rec;
      midi::cc14_reader<recorder&>     _chain{_rec};
      std::size_t                               _time = 0;
   };

   constexpr std::uint8_t modulation = 1;
   constexpr std::uint8_t modulation_lsb = 33;
   constexpr std::uint8_t expression = 11;
   constexpr std::uint8_t expression_lsb = 43;
}

TEST_CASE("A coarse half alone is a value in its own right")
{
   // A fine half may never come: most controllers send only the coarse one.
   // Waiting for a partner that will not arrive would swallow the move.
   fixture f;
   f.cc(0, modulation, 64);

   REQUIRE(f._rec._wide.size() == 1);
   CHECK(f._rec._wide.front().controller == modulation);
   CHECK(f._rec._wide.front().value == (64 << 7));
   CHECK(f._rec._narrow.empty());
}

TEST_CASE("The fine half completes the value")
{
   fixture f;
   f.cc(0, modulation, 64);
   f.cc(0, modulation_lsb, 100);

   REQUIRE(f._rec._wide.size() == 2);
   CHECK(f._rec._wide[0].value == (64 << 7));
   CHECK(f._rec._wide[1].value == ((64 << 7) | 100));
   CHECK(f._rec._wide[1].controller == modulation);
}

TEST_CASE("The fine half is reported under the coarse controller's number")
{
   // Controller 33 is not a controller of its own; it is the tail of 1.
   fixture f;
   f.cc(0, modulation, 1);
   f.cc(0, modulation_lsb, 1);

   for (auto const& w : f._rec._wide)
      CHECK(w.controller == modulation);
}

TEST_CASE("The full range is reachable")
{
   fixture f;
   f.cc(0, expression, 127);
   f.cc(0, expression_lsb, 127);

   CHECK(f._rec._wide.back().value == 16383);
}

TEST_CASE("A new coarse half clears the old fine one")
{
   // 64 then 100 gives 8292. A later coarse 65 alone means 8320, not
   // 8320 plus the 100 that was left behind.
   fixture f;
   f.cc(0, modulation, 64);
   f.cc(0, modulation_lsb, 100);
   f.cc(0, modulation, 65);

   CHECK(f._rec._wide.back().value == (65 << 7));
}

TEST_CASE("A fine half with no coarse half before it still reports")
{
   // Some controllers send the fine half on its own once the coarse one has
   // settled. The coarse half is whatever it last was, zero at the start.
   fixture f;
   f.cc(0, modulation_lsb, 42);

   REQUIRE(f._rec._wide.size() == 1);
   CHECK(f._rec._wide.front().controller == modulation);
   CHECK(f._rec._wide.front().value == 42);
}

TEST_CASE("Controllers keep their own halves")
{
   fixture f;
   f.cc(0, modulation, 10);
   f.cc(0, expression, 20);
   f.cc(0, modulation_lsb, 5);

   REQUIRE(f._rec._wide.size() == 3);
   CHECK(f._rec._wide[2].controller == modulation);
   CHECK(f._rec._wide[2].value == ((10 << 7) | 5));
}

TEST_CASE("Channels keep their own halves")
{
   fixture f;
   f.cc(0, modulation, 10);
   f.cc(1, modulation, 20);
   f.cc(1, modulation_lsb, 7);

   REQUIRE(f._rec._wide.size() == 3);
   CHECK(f._rec._wide[2].channel == 1);
   CHECK(f._rec._wide[2].value == ((20 << 7) | 7));
}

TEST_CASE("Controllers above the pairing range stay as they are")
{
   // 64 and up are switches, modes and the parameter controllers. Timbre,
   // which MPE rides on, is 74 and belongs here.
   fixture f;
   f.cc(0, 64, 127);       // sustain pedal
   f.cc(0, 74, 90);        // timbre

   CHECK(f._rec._wide.empty());
   REQUIRE(f._rec._narrow.size() == 2);
   CHECK(f._rec._narrow[0].controller == 64);
   CHECK(f._rec._narrow[1].controller == 74);
   CHECK(f._rec._narrow[1].value == 90);
}

TEST_CASE("Other messages pass through")
{
   fixture f;
   f.send(midi::note_on{0, 60, 100});
   f.cc(0, modulation, 64);

   CHECK(f._rec._notes == 1);
   CHECK(f._rec._wide.size() == 1);
}

TEST_CASE("Chained with parameters, each stage takes only its own")
{
   // The data entry controllers, 6 and 38, are a coarse and fine pair like
   // any other, so the order matters: rpn_reader must see them first, or
   // the parameter it is assembling never gets its value.
   recorder rec;
   auto chain = midi::rpn_reader{midi::cc14_reader{std::ref(rec)}};

   std::size_t time = 0;
   auto cc =
      [&](std::uint8_t channel, std::uint8_t ctrl, std::uint8_t value)
      {
         midi::control_change const msg{
            channel, midi::cc::controller(ctrl), value};
         midi::raw_message const raw{
            std::uint32_t(msg.data[0])
          | (std::uint32_t(msg.data[1]) << 8)
          | (std::uint32_t(msg.data[2]) << 16)};
         midi::dispatch(raw, time++, chain);
      };

   cc(0, 101, 0);          // select RPN 0, pitch bend sensitivity
   cc(0, 100, 0);
   cc(0, 6, 2);            // its value: 2 semitones
   cc(0, modulation, 64);  // and a mod wheel move, which is not a parameter

   REQUIRE(rec._params.size() == 1);
   CHECK(rec._params.front().number == 0);
   CHECK(rec._params.front().value == (2 << 7));

   REQUIRE(rec._wide.size() == 1);
   CHECK(rec._wide.front().controller == modulation);
   CHECK(rec._narrow.empty());
}
