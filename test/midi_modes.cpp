/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/modes.hpp>
#include <q/midi/controllers.hpp>

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

      void operator()(midi::all_sounds_off msg, std::size_t)
      {
         _seen.push_back("all_sounds_off");
         _channels.push_back(msg.channel());
      }

      void operator()(midi::reset_all_controllers msg, std::size_t)
      {
         _seen.push_back("reset_all_controllers");
         _channels.push_back(msg.channel());
      }

      void operator()(midi::local_control msg, std::size_t)
      {
         _seen.push_back("local_control");
         _channels.push_back(msg.channel());
         _local = msg.on();
      }

      void operator()(midi::all_notes_off msg, std::size_t)
      {
         _seen.push_back("all_notes_off");
         _channels.push_back(msg.channel());
      }

      void operator()(midi::omni_off msg, std::size_t)
      {
         _seen.push_back("omni_off");
         _channels.push_back(msg.channel());
      }

      void operator()(midi::omni_on msg, std::size_t)
      {
         _seen.push_back("omni_on");
         _channels.push_back(msg.channel());
      }

      void operator()(midi::mono_mode msg, std::size_t)
      {
         _seen.push_back("mono_mode");
         _channels.push_back(msg.channel());
         _voices = msg.channels();
      }

      void operator()(midi::poly_mode msg, std::size_t)
      {
         _seen.push_back("poly_mode");
         _channels.push_back(msg.channel());
      }

      void operator()(midi::control_change msg, std::size_t)
      {
         _ccs.push_back(std::uint8_t(msg.controller()));
      }

      void operator()(midi::control_change_14 msg, std::size_t)
      {
         _wide.push_back(std::uint8_t(msg.controller()));
      }

      void operator()(midi::note_on, std::size_t)
      {
         ++_notes;
      }

      std::vector<char const*>   _seen;
      std::vector<std::uint8_t>  _channels;
      std::vector<std::uint8_t>  _ccs;
      std::vector<std::uint8_t>  _wide;
      int                        _notes = 0;
      bool                       _local = false;
      std::uint8_t               _voices = 0;
   };

   struct fixture
   {
      void cc(std::uint8_t channel, std::uint8_t ctrl, std::uint8_t value)
      {
         midi::control_change const msg{
            channel, midi::cc::controller(ctrl), value};
         midi::raw_message const raw{
            std::uint32_t(msg.data[0])
          | (std::uint32_t(msg.data[1]) << 8)
          | (std::uint32_t(msg.data[2]) << 16)};
         midi::dispatch(raw, _time++, _chain);
      }

      recorder                            _rec;
      midi::mode_reader<recorder&>        _chain{_rec};
      std::size_t                         _time = 0;
   };
}

TEST_CASE("Each mode controller arrives as its own message")
{
   fixture f;
   f.cc(0, 120, 0);        // all sounds off
   f.cc(0, 121, 0);        // reset all controllers
   f.cc(0, 123, 0);        // all notes off
   f.cc(0, 124, 0);        // omni off
   f.cc(0, 125, 0);        // omni on
   f.cc(0, 127, 0);        // poly

   REQUIRE(f._rec._seen.size() == 6);
   CHECK(std::string(f._rec._seen[0]) == "all_sounds_off");
   CHECK(std::string(f._rec._seen[1]) == "reset_all_controllers");
   CHECK(std::string(f._rec._seen[2]) == "all_notes_off");
   CHECK(std::string(f._rec._seen[3]) == "omni_off");
   CHECK(std::string(f._rec._seen[4]) == "omni_on");
   CHECK(std::string(f._rec._seen[5]) == "poly_mode");

   // And none of them reached the processor as a plain controller.
   CHECK(f._rec._ccs.empty());
}

TEST_CASE("The channel comes along")
{
   fixture f;
   f.cc(9, 123, 0);

   REQUIRE(f._rec._channels.size() == 1);
   CHECK(f._rec._channels.front() == 9);
}

TEST_CASE("Local control carries its switch")
{
   // 0 is off, 127 is on, and the specification defines nothing between.
   fixture f;
   f.cc(0, 122, 127);
   CHECK(f._rec._local);

   f.cc(0, 122, 0);
   CHECK_FALSE(f._rec._local);

   CHECK(f._rec._seen.size() == 2);
}

TEST_CASE("Mono mode carries its voice count")
{
   // The value is how many channels the instrument should answer on, and
   // zero means every channel it has. This is the message MPE reuses.
   fixture f;
   f.cc(0, 126, 4);
   CHECK(f._rec._voices == 4);

   f.cc(0, 126, 0);
   CHECK(f._rec._voices == 0);
}

TEST_CASE("Ordinary controllers are untouched")
{
   fixture f;
   f.cc(0, 7, 100);        // channel volume
   f.cc(0, 74, 64);        // timbre
   f.cc(0, 64, 127);       // sustain pedal

   CHECK(f._rec._seen.empty());
   REQUIRE(f._rec._ccs.size() == 3);
   CHECK(f._rec._ccs[0] == 7);
   CHECK(f._rec._ccs[1] == 74);
   CHECK(f._rec._ccs[2] == 64);
}

TEST_CASE("Other messages pass through")
{
   fixture f;
   midi::note_on const on{0, 60, 100};
   midi::raw_message const raw{
      std::uint32_t(on.data[0])
    | (std::uint32_t(on.data[1]) << 8)
    | (std::uint32_t(on.data[2]) << 16)};
   midi::dispatch(raw, 0, f._chain);

   CHECK(f._rec._notes == 1);
}

TEST_CASE("Chained with the controller reader, each takes its own")
{
   // Mode controllers are 120 and up, the pairing range is 0 to 63, so the
   // two stages cannot collide whichever way round they are nested.
   recorder rec;
   auto chain = midi::mode_reader{midi::cc14_reader{std::ref(rec)}};

   std::size_t time = 0;
   auto cc =
      [&](std::uint8_t ctrl, std::uint8_t value)
      {
         midi::control_change const msg{
            0, midi::cc::controller(ctrl), value};
         midi::raw_message const raw{
            std::uint32_t(msg.data[0])
          | (std::uint32_t(msg.data[1]) << 8)
          | (std::uint32_t(msg.data[2]) << 16)};
         midi::dispatch(raw, time++, chain);
      };

   cc(1, 64);              // modulation, which the pairing stage takes
   cc(123, 0);             // all notes off, which this stage takes

   REQUIRE(rec._wide.size() == 1);
   CHECK(rec._wide.front() == 1);
   REQUIRE(rec._seen.size() == 1);
   CHECK(std::string(rec._seen.front()) == "all_notes_off");
}
