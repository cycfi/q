/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Conformance tests taken from MIDI Polyphonic Expression, version 1.0,
// March 12 2018 (MMA/AMEI RP-053). Each case names the section it comes
// from, and asserts the rule rather than our reading of it.
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
      };

      struct expression
      {
         std::uint8_t   channel;
         std::uint8_t   key;
         float          value;
      };

      void operator()(midi::note_on msg, std::size_t)
      {
         _on.push_back({msg.channel(), msg.key()});
      }

      void operator()(midi::note_off msg, std::size_t)
      {
         _off.push_back({msg.channel(), msg.key()});
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

      void operator()(midi::poly_aftertouch msg, std::size_t)
      {
         _poly.push_back(msg.channel());
      }

      void operator()(midi::control_change msg, std::size_t)
      {
         _ccs.push_back(std::uint8_t(msg.controller()));
      }

      void operator()(midi::program_change msg, std::size_t)
      {
         _programs.push_back(msg.channel());
      }

      // The last value reported for a key, whatever channel carried it.
      float pitch_of(std::uint8_t key) const
      {
         for (auto i = _pitch.rbegin(); i != _pitch.rend(); ++i)
            if (i->key == key)
               return i->value;
         return 0.0f;
      }

      std::vector<note>          _on;
      std::vector<note>          _off;
      std::vector<expression>    _pitch;
      std::vector<expression>    _pressure;
      std::vector<expression>    _timbre;
      std::vector<std::uint8_t>  _bends;
      std::vector<std::uint8_t>  _aftertouch;
      std::vector<std::uint8_t>  _poly;
      std::vector<std::uint8_t>  _ccs;
      std::vector<std::uint8_t>  _programs;
   };

   constexpr std::uint16_t centre = 8192;

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
      template <typename Message>
      void send(Message const& msg)
      {
         midi::raw_message const raw = to_raw(msg);
         midi::dispatch(raw, _time++, _chain);
      }

      void cc(std::uint8_t channel, std::uint8_t ctrl, std::uint8_t value)
      {
         send(midi::control_change{
            channel, midi::cc::controller(ctrl), value});
      }

      // Section 2.1.1: "Message Format: [Bn 64 06] [Bn 65 00] [Bn 06 <mm>]",
      // that is, the parameter's low half first.
      void mcm(std::uint8_t channel, std::uint8_t members)
      {
         cc(channel, 0x64, 0x06);
         cc(channel, 0x65, 0x00);
         cc(channel, 0x06, members);
      }

      void bend_range(std::uint8_t channel, std::uint8_t semitones)
      {
         cc(channel, 0x64, 0x00);
         cc(channel, 0x65, 0x00);
         cc(channel, 0x06, semitones);
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

// 2.1.1 MPE Configuration Message ////////////////////////////////////////////

TEST_CASE("2.1.1 The configuration message is registered parameter 00 06")
{
   fixture f;
   f.mcm(0, 4);
   CHECK(f._chain.lower_members() == 4);
}

TEST_CASE("2.1.1 Only channels 1 and 16 may carry it; others are ignored")
{
   // "n=0: Lower Zone Master Channel, n=F: Upper Zone Master Channel.
   // All other values are invalid and should be ignored"
   fixture f;
   f.mcm(3, 4);

   CHECK(f._chain.lower_members() == 0);
   CHECK(f._chain.upper_members() == 0);
}

TEST_CASE("2.1.1 A member count of zero switches the zone off")
{
   fixture f;
   f.mcm(0, 4);
   f.mcm(0, 0);
   CHECK(f._chain.lower_members() == 0);
}

TEST_CASE("2.1.1 The lower zone counts up, the upper zone counts down")
{
   fixture f;
   f.mcm(0, 3);            // master 1, members 2 to 4
   f.mcm(15, 3);           // master 16, members 15 down to 13

   f.send(midi::note_on{3, 60, 100});     // channel 4, in the lower zone
   f.send(midi::note_on{12, 72, 100});    // channel 13, in the upper zone
   f._rec._pitch.clear();

   f.bend(3, centre + 4096);
   f.bend(12, centre + 4096);

   REQUIRE(f._rec._pitch.size() == 2);
   CHECK(f._rec._pitch[0].key == 60);
   CHECK(f._rec._pitch[1].key == 72);
}

TEST_CASE("2.1.1 One zone may hold fifteen members, taking the other master")
{
   // "The Master Channel of an unused Zone can be used as a Member Channel
   // for the other Zone. Thus, if only one Zone is active, it may use up to
   // 15 Member Channels"
   fixture f;
   f.mcm(0, 15);           // master 1, members 2 to 16

   f.send(midi::note_on{15, 60, 100});    // channel 16, a member here
   f._rec._pitch.clear();
   f.bend(15, centre + 4096);

   REQUIRE(f._rec._pitch.size() == 1);
   CHECK(f._rec._pitch.front().channel == 15);
}

TEST_CASE("2.1.1 A member count above fifteen is invalid")
{
   fixture f;
   f.mcm(0, 16);
   CHECK(f._chain.lower_members() == 0);
}

TEST_CASE("2.1.1 The newer message wins the channels it claims")
{
   // "in the case where an MCM configures a Zone to include MIDI Channels
   // that were previously assigned to another Zone, the most recent message
   // takes precedence"
   fixture f;
   f.mcm(0, 15);           // lower takes members 2 to 16
   f.mcm(15, 4);           // upper claims master 16 and members 15 to 12

   CHECK(f._chain.upper_members() == 4);
   CHECK(f._chain.lower_members() == 10);    // shrunk to members 2 to 11
}

TEST_CASE("2.1.1 The newer message may take a zone away entirely")
{
   // "even if this results in deactivating a Zone"
   fixture f;
   f.mcm(15, 4);           // upper: master 16, members 15 to 12
   f.mcm(0, 15);           // lower claims every channel above 1

   CHECK(f._chain.lower_members() == 15);
   CHECK(f._chain.upper_members() == 0);
}

// 2.1.4 Receiver behaviour when resetting zones //////////////////////////////

TEST_CASE("2.1.4 Changing a zone stops every note it was holding")
{
   // "receivers are required to stop all ongoing notes and reset all
   // controls to reasonable default values on each Channel entering or
   // leaving MPE control"
   fixture f;
   f.mcm(0, 4);
   f.send(midi::note_on{1, 60, 100});
   f.send(midi::note_on{2, 64, 100});
   f._rec._off.clear();

   f.mcm(0, 8);

   REQUIRE(f._rec._off.size() == 2);
   CHECK(f._rec._off[0].key == 60);
   CHECK(f._rec._off[1].key == 64);
}

TEST_CASE("2.4 A configuration message restores both bend ranges")
{
   // "When a receiver receives an MPE Configuration Message, it must set
   // the Master Pitch Bend Sensitivity to +/-2 semitones, and the Pitch
   // Bend Sensitivity of the Member Channels to +/-48 semitones."
   fixture f;
   f.mcm(0, 4);
   f.bend_range(1, 12);
   f.mcm(0, 4);            // sent again: the range goes back to 48

   f.send(midi::note_on{1, 60, 100});
   f.bend(1, centre + 4096);

   CHECK(f._rec.pitch_of(60) == Approx(24.0f).margin(0.01));
}

// 2.4 Pitch bend /////////////////////////////////////////////////////////////

TEST_CASE("2.4 The last range sent to any member applies to them all")
{
   // "A receiver must apply the last Pitch Bend Sensitivity message
   // received on any Member Channel to all Member Channels in the Zone."
   fixture f;
   f.mcm(0, 4);
   f.bend_range(1, 12);          // sent to one member only

   f.send(midi::note_on{3, 60, 100});
   f.bend(3, centre + 4096);     // a different member

   CHECK(f._rec.pitch_of(60) == Approx(6.0f).margin(0.01));
}

TEST_CASE("2.4 The master's range is set on the master channel alone")
{
   fixture f;
   f.mcm(0, 4);
   f.bend_range(0, 12);          // the zone's own range

   f.send(midi::note_on{1, 60, 100});
   f.bend(1, centre + 4096);     // still 48 semitones on a member

   CHECK(f._rec.pitch_of(60) == Approx(24.0f).margin(0.01));
}

TEST_CASE("2.4 Master and member bend combine for each sounding note")
{
   // "If a device receives Pitch Bend on both a Master Channel and Member
   // Channel, it must combine such data meaningfully and separately for
   // each sounding note."
   fixture f;
   f.mcm(0, 4);
   f.bend_range(0, 2);
   f.send(midi::note_on{1, 60, 100});
   f.send(midi::note_on{2, 64, 100});

   f.bend(1, centre + 4096);     // this note only, +24
   f.bend(0, centre + 4096);     // the zone, +1

   CHECK(f._rec.pitch_of(60) == Approx(25.0f).margin(0.01));
   CHECK(f._rec.pitch_of(64) == Approx(1.0f).margin(0.01));
}

// 2.5 Channel pressure and polyphonic key pressure ///////////////////////////

TEST_CASE("2.5 Master and member pressure combine for each sounding note")
{
   fixture f;
   f.mcm(0, 4);
   f.send(midi::note_on{1, 60, 100});

   f.send(midi::channel_aftertouch{1, 64});     // about half, on the note
   f.send(midi::channel_aftertouch{0, 63});     // about half, on the zone

   REQUIRE(!f._rec._pressure.empty());
   CHECK(f._rec._pressure.back().value == Approx(1.0f).margin(0.02));
}

TEST_CASE("2.5 Polyphonic key pressure is not read on a member channel")
{
   // "Polyphonic Key Pressure must not be sent on Member Channels. It is
   // currently reserved for a future extension of this specification."
   fixture f;
   f.mcm(0, 4);
   f.send(midi::note_on{1, 60, 100});
   f._rec._pressure.clear();      // the note's initial state, not this rule
   f.send(midi::poly_aftertouch{1, 60, 100});

   CHECK(f._rec._pressure.empty());
   CHECK(f._rec._poly.empty());
}

TEST_CASE("2.5 Polyphonic key pressure on the master channel is allowed")
{
   // "Polyphonic Key Pressure may be sent for notes on the Master Channel
   // at the discretion of the implementer"
   fixture f;
   f.mcm(0, 4);
   f.send(midi::poly_aftertouch{0, 60, 100});

   REQUIRE(f._rec._poly.size() == 1);
   CHECK(f._rec._poly.front() == 0);
}

// 2.6 The third dimension ////////////////////////////////////////////////////

TEST_CASE("2.6 Master and member timbre combine for each sounding note")
{
   fixture f;
   f.mcm(0, 4);
   f.send(midi::note_on{1, 60, 100});

   f.cc(1, 74, 96);        // above centre on the note
   f.cc(0, 74, 96);        // and above centre on the zone

   REQUIRE(!f._rec._timbre.empty());
   CHECK(f._rec._timbre.back().value > 0.75f);
}

// 2.3 Zone messages against note level messages //////////////////////////////

TEST_CASE("2.3.1 A zone message on a member channel is ignored")
{
   // "If an MPE synthesizer receives one of those messages on a Member
   // Channel, it must ignore it." The damper pedal is such a message.
   fixture f;
   f.mcm(0, 4);
   f.cc(1, 64, 127);       // damper pedal, on a member

   CHECK(f._rec._ccs.empty());
}

TEST_CASE("2.3.1 The same message on the master channel is kept")
{
   fixture f;
   f.mcm(0, 4);
   f.cc(0, 64, 127);

   REQUIRE(f._rec._ccs.size() == 1);
   CHECK(f._rec._ccs.front() == 64);
}

TEST_CASE("2.3.3 A program change on a member channel is ignored")
{
   // "a receiver operating in Mode 3 should ignore Program Change messages
   // received on Member Channels", and mode 3 is MPE's usual mode.
   fixture f;
   f.mcm(0, 4);
   f.send(midi::program_change{1, 7});
   f.send(midi::program_change{0, 7});

   REQUIRE(f._rec._programs.size() == 1);
   CHECK(f._rec._programs.front() == 0);
}

// 2.2.1 and 3.3 Channels, notes and initial state ////////////////////////////

TEST_CASE("2.2.1 Two notes on one channel both answer to it")
{
   // "When there are more Notes than unoccupied Channels, a new note must
   // share a MIDI Channel with an existing note. Since Control Change and
   // Pitch Bend are Channel Messages, they then affect both notes."
   fixture f;
   f.mcm(0, 4);
   f.send(midi::note_on{1, 60, 100});
   f.send(midi::note_on{1, 64, 100});

   f.bend(1, centre + 4096);

   CHECK(f._rec.pitch_of(60) == Approx(24.0f).margin(0.01));
   CHECK(f._rec.pitch_of(64) == Approx(24.0f).margin(0.01));
}

TEST_CASE("3.3 A bend before the note sets the note's initial pitch")
{
   // "The pitch of a new note is influenced by the Pitch Bend message most
   // recently received on its Channel before Note On, so a synthesizer must
   // continue to track it even when no note is playing."
   fixture f;
   f.mcm(0, 4);
   f.bend(1, centre + 4096);        // before any note
   CHECK(f._rec._pitch.empty());

   f.send(midi::note_on{1, 60, 100});

   REQUIRE(!f._rec._pitch.empty());
   CHECK(f._rec.pitch_of(60) == Approx(24.0f).margin(0.01));
}

TEST_CASE("3.3 Master bend also sets a new note's initial pitch")
{
   // "Master Channel Pitch Bend also influences the note's initial pitch."
   fixture f;
   f.mcm(0, 4);
   f.bend_range(0, 2);
   f.bend(0, centre + 4096);        // the zone, before any note

   f.send(midi::note_on{1, 60, 100});

   CHECK(f._rec.pitch_of(60) == Approx(1.0f).margin(0.01));
}

TEST_CASE("3.3 Pressure and timbre are tracked before the note too")
{
   // "Values for all these must be tracked and stored on all Member
   // Channels, even when no note is playing, to provide an initial state
   // for a new note."
   fixture f;
   f.mcm(0, 4);
   f.send(midi::channel_aftertouch{1, 127});
   f.cc(1, 74, 127);

   f.send(midi::note_on{1, 60, 100});

   REQUIRE(!f._rec._pressure.empty());
   CHECK(f._rec._pressure.back().value == Approx(1.0f).margin(0.01));
   REQUIRE(!f._rec._timbre.empty());
   CHECK(f._rec._timbre.back().value == Approx(1.0f).margin(0.01));
}

TEST_CASE("3.3.5 Timbre starts centred when nothing has been sent")
{
   // "the initial position of CC #74 under such circumstances must be 40h
   // (64 decimal), such that movement can follow in either a positive or
   // negative direction."
   fixture f;
   f.mcm(0, 4);
   f.send(midi::note_on{1, 60, 100});

   REQUIRE(!f._rec._timbre.empty());
   CHECK(f._rec._timbre.back().value == Approx(0.5f).margin(0.01));
}

TEST_CASE("3.3 A note stops answering to its channel once it has ended")
{
   // "The note will cease to be affected by Pitch Bend messages on its
   // Channel after the Note Off message occurs."
   fixture f;
   f.mcm(0, 4);
   f.send(midi::note_on{1, 60, 100});
   f.send(midi::note_off{1, 60, 0});
   f._rec._pitch.clear();

   f.bend(1, 0);

   CHECK(f._rec._pitch.empty());
}

TEST_CASE("3.3 The channel keeps that value for the next note")
{
   fixture f;
   f.mcm(0, 4);
   f.send(midi::note_on{1, 60, 100});
   f.send(midi::note_off{1, 60, 0});
   f.bend(1, centre + 4096);        // with nothing sounding
   f.send(midi::note_on{1, 67, 100});

   CHECK(f._rec.pitch_of(67) == Approx(24.0f).margin(0.01));
}

TEST_CASE("Channels outside every zone are untouched")
{
   fixture f;
   f.mcm(0, 3);            // members 2 to 4 only
   f.send(midi::note_on{8, 60, 100});
   f.bend(8, 0);
   f.cc(8, 74, 100);

   CHECK(f._rec._bends.size() == 1);
   CHECK(f._rec._ccs.size() == 1);
   CHECK(f._rec._pitch.empty());
}
