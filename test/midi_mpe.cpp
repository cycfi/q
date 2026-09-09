/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/mpe.hpp>

#include <cstdint>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;

namespace
{
   struct recorder : midi::processor
   {
      using midi::processor::operator();

      struct note
      {
         std::uint8_t   channel;
         std::uint8_t   key;
         std::uint8_t   velocity;
      };

      struct expression
      {
         std::uint8_t   channel;
         std::uint8_t   key;
         float          value;
      };

      void operator()(midi::note_on msg, std::size_t)
      {
         _on.push_back({msg.channel(), msg.key(), msg.velocity()});
      }

      void operator()(midi::note_off msg, std::size_t)
      {
         _off.push_back({msg.channel(), msg.key(), msg.velocity()});
      }

      void operator()(midi::note_pitch msg, std::size_t)
      {
         _pitch.push_back({msg.channel(), msg.key(), msg.semitones()});
      }

      void operator()(midi::note_pressure msg, std::size_t)
      {
         _pressure.push_back({msg.channel(), msg.key(), msg.value()});
      }

      void operator()(midi::note_timbre msg, std::size_t)
      {
         _timbre.push_back({msg.channel(), msg.key(), msg.value()});
      }

      void operator()(midi::pitch_bend msg, std::size_t)
      {
         _bends.push_back(msg.channel());
      }

      void operator()(midi::channel_aftertouch msg, std::size_t)
      {
         _aftertouch.push_back(msg.channel());
      }

      void operator()(midi::control_change msg, std::size_t)
      {
         _ccs.push_back(std::uint8_t(msg.controller()));
      }

      std::vector<note>          _on;
      std::vector<note>          _off;
      std::vector<expression>    _pitch;
      std::vector<expression>    _pressure;
      std::vector<expression>    _timbre;
      std::vector<std::uint8_t>  _bends;
      std::vector<std::uint8_t>  _aftertouch;
      std::vector<std::uint8_t>  _ccs;
   };

   constexpr std::uint8_t timbre_cc = 74;
   constexpr std::uint16_t bend_centre = 8192;

   struct fixture
   {
      template <typename Message>
      void send(Message const& msg)
      {
         midi::raw_message const raw{
            std::uint32_t(msg.data[0])
          | (std::uint32_t(msg.data[1]) << 8)
          | (std::uint32_t(msg.data[2]) << 16)};
         midi::dispatch(raw, _time++, _chain);
      }

      void cc(std::uint8_t channel, std::uint8_t ctrl, std::uint8_t value)
      {
         send(midi::control_change{
            channel, midi::cc::controller(ctrl), value});
      }

      // The configuration message: registered parameter 6 on the zone's
      // master channel, its value the number of member channels.
      void configure(std::uint8_t master, std::uint8_t members)
      {
         cc(master, 101, 0);
         cc(master, 100, 6);
         cc(master, 6, members);
      }

      // Registered parameter 0, the bend range in semitones.
      void bend_range(std::uint8_t channel, std::uint8_t semitones)
      {
         cc(channel, 101, 0);
         cc(channel, 100, 0);
         cc(channel, 6, semitones);
      }

      // A note on now reports the channel's current state as the note's
      // initial state, per section 3.3. These tests are about what happens
      // after that, so the note starts and the slate is wiped.
      void note(std::uint8_t channel, std::uint8_t key, std::uint8_t velocity)
      {
         send(midi::note_on{channel, key, velocity});
         _rec._pitch.clear();
         _rec._pressure.clear();
         _rec._timbre.clear();
      }

      void bend(std::uint8_t channel, std::uint16_t value)
      {
         send(midi::pitch_bend{channel, value});
      }

      recorder                      _rec;
      midi::mpe_reader<recorder&>   _chain{_rec};
      std::size_t                   _time = 0;
   };
}

TEST_CASE("Without a zone, nothing is per note")
{
   // A plain keyboard on one channel must play exactly as it did before.
   fixture f;
   f.note(0, 60, 100);
   f.bend(0, 10000);
   f.send(midi::channel_aftertouch{0, 64});
   f.cc(0, timbre_cc, 100);

   CHECK(f._rec._on.size() == 1);
   CHECK(f._rec._bends.size() == 1);
   CHECK(f._rec._aftertouch.size() == 1);
   CHECK(f._rec._ccs.size() == 1);
   CHECK(f._rec._pitch.empty());
   CHECK(f._rec._pressure.empty());
   CHECK(f._rec._timbre.empty());
}

TEST_CASE("A lower zone claims the channels above its master")
{
   fixture f;
   f.configure(0, 4);              // master 0, members 1 to 4

   CHECK(f._chain.zone_members() == 4);

   // The configuration itself is not passed on as controllers.
   CHECK(f._rec._ccs.empty());
}

TEST_CASE("A member channel's bend belongs to the note it is playing")
{
   fixture f;
   f.configure(0, 4);
   f.note(1, 60, 100);

   // Half way up, with the MPE default range of 48 semitones.
   f.bend(1, bend_centre + 4096);

   REQUIRE(f._rec._pitch.size() == 1);
   CHECK(f._rec._pitch.front().channel == 1);
   CHECK(f._rec._pitch.front().key == 60);
   CHECK(f._rec._pitch.front().value == Approx(24.0f).margin(0.01));

   // And it did not also arrive as a plain bend.
   CHECK(f._rec._bends.empty());
}

TEST_CASE("Bend follows the range the zone was given")
{
   fixture f;
   f.configure(0, 4);
   f.bend_range(1, 12);
   f.note(1, 60, 100);
   f.bend(1, bend_centre + 4096);

   REQUIRE(f._rec._pitch.size() == 1);
   CHECK(f._rec._pitch.front().value == Approx(6.0f).margin(0.01));
}

TEST_CASE("Bending down is negative")
{
   fixture f;
   f.configure(0, 4);
   f.bend_range(1, 2);
   f.note(1, 60, 100);
   f.bend(1, 0);

   REQUIRE(f._rec._pitch.size() == 1);
   CHECK(f._rec._pitch.front().value == Approx(-2.0f).margin(0.01));
}

TEST_CASE("Pressure and timbre belong to the note too")
{
   fixture f;
   f.configure(0, 4);
   f.note(2, 64, 100);
   f.send(midi::channel_aftertouch{2, 127});
   f.cc(2, timbre_cc, 0);

   REQUIRE(f._rec._pressure.size() == 1);
   CHECK(f._rec._pressure.front().key == 64);
   CHECK(f._rec._pressure.front().value == Approx(1.0f).margin(0.01));

   REQUIRE(f._rec._timbre.size() == 1);
   CHECK(f._rec._timbre.front().key == 64);
   CHECK(f._rec._timbre.front().value == Approx(0.0f).margin(0.01));

   CHECK(f._rec._aftertouch.empty());
   CHECK(f._rec._ccs.empty());
}

TEST_CASE("Expression with no note sounding goes nowhere")
{
   fixture f;
   f.configure(0, 4);
   f.bend(1, 0);
   f.send(midi::channel_aftertouch{1, 100});

   CHECK(f._rec._pitch.empty());
   CHECK(f._rec._pressure.empty());
}

TEST_CASE("A note ending releases its channel")
{
   fixture f;
   f.configure(0, 4);
   f.note(1, 60, 100);
   f.send(midi::note_off{1, 60, 0});
   f.bend(1, 0);

   CHECK(f._rec._off.size() == 1);
   CHECK(f._rec._pitch.empty());
}

TEST_CASE("A note on with zero velocity ends the note")
{
   // The old way of saying note off, and controllers still send it.
   fixture f;
   f.configure(0, 4);
   f.note(1, 60, 100);
   f.send(midi::note_on{1, 60, 0});
   f.bend(1, 0);

   CHECK(f._rec._pitch.empty());
}

TEST_CASE("Each member channel carries its own note")
{
   fixture f;
   f.configure(0, 4);
   f.note(1, 60, 100);
   f.note(2, 64, 100);

   f.bend(1, bend_centre + 4096);
   f.bend(2, bend_centre - 4096);

   REQUIRE(f._rec._pitch.size() == 2);
   CHECK(f._rec._pitch[0].key == 60);
   CHECK(f._rec._pitch[0].value == Approx(24.0f).margin(0.01));
   CHECK(f._rec._pitch[1].key == 64);
   CHECK(f._rec._pitch[1].value == Approx(-24.0f).margin(0.01));
}

TEST_CASE("The master channel bends the whole zone")
{
   // A master move applies to every sounding note, on top of whatever each
   // note is doing on its own.
   fixture f;
   f.configure(0, 4);
   f.bend_range(0, 2);           // the master's own range
   f.bend_range(1, 48);
   f.note(1, 60, 100);
   f.note(2, 64, 100);

   f.bend(0, bend_centre + 4096);   // master, half way up

   REQUIRE(f._rec._pitch.size() == 2);
   CHECK(f._rec._pitch[0].key == 60);
   CHECK(f._rec._pitch[0].value == Approx(1.0f).margin(0.01));
   CHECK(f._rec._pitch[1].key == 64);
   CHECK(f._rec._pitch[1].value == Approx(1.0f).margin(0.01));
}

TEST_CASE("Master and member bends add")
{
   fixture f;
   f.configure(0, 4);
   f.bend_range(0, 2);
   f.bend_range(1, 48);
   f.note(1, 60, 100);

   f.bend(0, bend_centre + 4096);      // +1 semitone across the zone
   f.bend(1, bend_centre + 4096);      // +24 on this note

   REQUIRE(f._rec._pitch.size() == 2);
   CHECK(f._rec._pitch.back().value == Approx(25.0f).margin(0.01));
}

TEST_CASE("An upper zone counts down from channel 16")
{
   // Master 15, members 14 downwards, which is how the two zones share a
   // cable without meeting.
   fixture f;
   f.configure(15, 3);
   f.note(14, 72, 100);
   f.bend(14, bend_centre + 4096);

   REQUIRE(f._rec._pitch.size() == 1);
   CHECK(f._rec._pitch.front().channel == 14);
   CHECK(f._rec._pitch.front().key == 72);
}

TEST_CASE("Channels outside the zone are left alone")
{
   fixture f;
   f.configure(0, 4);              // members 1 to 4
   f.note(7, 60, 100);
   f.bend(7, 0);

   CHECK(f._rec._on.size() == 1);
   CHECK(f._rec._bends.size() == 1);
   CHECK(f._rec._pitch.empty());
}

TEST_CASE("A zone of no members switches MPE off again")
{
   fixture f;
   f.configure(0, 4);
   f.note(1, 60, 100);
   f.configure(0, 0);

   CHECK(f._chain.zone_members() == 0);

   f.bend(1, 0);
   CHECK(f._rec._pitch.empty());
   CHECK(f._rec._bends.size() == 1);
}

TEST_CASE("Notes pass through whole")
{
   // The synth still gets its note on and note off; only the expression
   // changes shape.
   fixture f;
   f.configure(0, 4);
   f.note(1, 60, 100);
   f.send(midi::note_off{1, 60, 40});

   REQUIRE(f._rec._on.size() == 1);
   CHECK(f._rec._on.front().key == 60);
   CHECK(f._rec._on.front().velocity == 100);
   REQUIRE(f._rec._off.size() == 1);
   CHECK(f._rec._off.front().velocity == 40);
}
