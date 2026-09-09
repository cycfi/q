/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Default Translation Mode, M2-104-UM Appendix D. D.2 is MIDI 2.0 to 1.0,
// D.3 is MIDI 1.0 to 2.0. Scaling per M2-115-U. Cases are named for their
// clause.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/translate.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;

namespace
{
   // What a MIDI 1.0 processor hears after translation down.
   struct midi1_recorder : midi::processor
   {
      using midi::processor::operator();

      struct entry
      {
         std::string    kind;
         std::uint8_t   channel;
         std::uint32_t  a;
         std::uint32_t  b;
      };

      void operator()(midi::note_on m, std::size_t)
      { _log.push_back({"note_on", m.channel(), m.key(), m.velocity()}); }
      void operator()(midi::note_off m, std::size_t)
      { _log.push_back({"note_off", m.channel(), m.key(), m.velocity()}); }
      void operator()(midi::poly_aftertouch m, std::size_t)
      { _log.push_back({"poly", m.channel(), m.key(), m.pressure()}); }
      void operator()(midi::control_change m, std::size_t)
      {
         _log.push_back(
            {"cc", m.channel(), std::uint8_t(m.controller()), m.value()});
      }
      void operator()(midi::program_change m, std::size_t)
      { _log.push_back({"program", m.channel(), m.preset(), 0}); }
      void operator()(midi::channel_aftertouch m, std::size_t)
      { _log.push_back({"pressure", m.channel(), m.pressure(), 0}); }
      void operator()(midi::pitch_bend m, std::size_t)
      { _log.push_back({"bend", m.channel(), m.value(), 0}); }
      void operator()(midi::timing_tick, std::size_t)
      { _log.push_back({"tick", 0, 0, 0}); }

      std::vector<entry> _log;
   };

   // What a MIDI 2.0 processor hears after translation up: the packet
   // words, so a test can compare against Table 19 exactly.
   struct midi2_recorder : midi2::processor
   {
      using midi2::processor::operator();

      struct entry
      {
         std::string    kind;
         std::uint32_t  w0;
         std::uint32_t  w1;
      };

      template <typename M>
      void record(char const* kind, M const& m)
      {
         _log.push_back({kind, m.word(0), m.word(1)});
      }

      void operator()(midi2::note_on m, std::size_t)
      { record("note_on", m); }
      void operator()(midi2::note_off m, std::size_t)
      { record("note_off", m); }
      void operator()(midi2::poly_pressure m, std::size_t)
      { record("poly", m); }
      void operator()(midi2::control_change m, std::size_t)
      { record("cc", m); }
      void operator()(midi2::registered_controller m, std::size_t)
      { record("rpn", m); }
      void operator()(midi2::assignable_controller m, std::size_t)
      { record("nrpn", m); }
      void operator()(midi2::program_change m, std::size_t)
      { record("program", m); }
      void operator()(midi2::channel_pressure m, std::size_t)
      { record("pressure", m); }
      void operator()(midi2::pitch_bend m, std::size_t)
      { record("bend", m); }
      void operator()(midi::timing_tick, std::size_t)
      { _log.push_back({"tick", 0, 0}); }

      std::vector<entry> _log;
   };

   struct down_fixture
   {
      void send(midi2::packet const& p)
      {
         midi2::dispatch(p, _time++, _chain);
      }

      midi1_recorder                   _rec;
      midi2::to_midi1<midi1_recorder&> _chain{_rec};
      std::size_t                      _time = 0;
   };

   struct up_fixture
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

      void cc(std::uint8_t ch, std::uint8_t ctrl, std::uint8_t value)
      {
         send(midi::control_change{ch, midi::cc::controller(ctrl), value});
      }

      midi2_recorder                   _rec;
      midi2::to_midi2<midi2_recorder&> _chain{_rec};
      std::size_t                      _time = 0;
   };
}

// D.2 MIDI 2.0 to MIDI 1.0 ///////////////////////////////////////////////////

TEST_CASE("D.2.1 Note on: velocity scales 16 to 7, and never to zero")
{
   // "if the translated MIDI 1.0 value of the Velocity is 0, replace the
   // value with 1."
   down_fixture f;
   f.send({0x40913C00u, 0xFFFF0000u});     // channel 1, key 60, full
   f.send({0x40913C00u, 0x00010000u});     // nearly silent

   REQUIRE(f._rec._log.size() == 2);
   CHECK(f._rec._log[0].kind == "note_on");
   CHECK(f._rec._log[0].channel == 1);
   CHECK(f._rec._log[0].a == 60);
   CHECK(f._rec._log[0].b == 127);
   CHECK(f._rec._log[1].b == 1);
}

TEST_CASE("D.2.1 Note off keeps a zero velocity")
{
   down_fixture f;
   f.send({0x40803C00u, 0x00000000u});

   REQUIRE(f._rec._log.size() == 1);
   CHECK(f._rec._log[0].kind == "note_off");
   CHECK(f._rec._log[0].b == 0);
}

TEST_CASE("D.2.1 Poly pressure and control change scale 32 to 7")
{
   down_fixture f;
   f.send({0x40A03C00u, 0x80000000u});     // key 60, centre
   f.send({0x40B00700u, 0xFFFFFFFFu});     // controller 7, full

   REQUIRE(f._rec._log.size() == 2);
   CHECK(f._rec._log[0].kind == "poly");
   CHECK(f._rec._log[0].b == 64);
   CHECK(f._rec._log[1].kind == "cc");
   CHECK(f._rec._log[1].a == 7);
   CHECK(f._rec._log[1].b == 127);
}

TEST_CASE("D.2.2 Channel pressure scales 32 to 7")
{
   down_fixture f;
   f.send({0x40D00000u, 0x40000000u});

   REQUIRE(f._rec._log.size() == 1);
   CHECK(f._rec._log[0].kind == "pressure");
   CHECK(f._rec._log[0].a == 32);
}

TEST_CASE("D.2.3 A registered controller becomes four control changes")
{
   // "each message generates a sequence of four MIDI 1.0 Protocol
   // messages". Bank 0 index 0 is pitch bend sensitivity, zero extended
   // per M2-115 4.1: 12 semitones is a data entry MSB of 12, the 14 bit
   // value 1536, which is 0x18000000 once shifted up 18 bits. It must
   // come back as 12.
   down_fixture f;
   f.send({0x40200000u, 0x18000000u});

   REQUIRE(f._rec._log.size() == 4);
   CHECK(f._rec._log[0].a == 101); CHECK(f._rec._log[0].b == 0);
   CHECK(f._rec._log[1].a == 100); CHECK(f._rec._log[1].b == 0);
   CHECK(f._rec._log[2].a == 6);   CHECK(f._rec._log[2].b == 12);
   CHECK(f._rec._log[3].a == 38);  CHECK(f._rec._log[3].b == 0);
}

TEST_CASE("D.2.3 An assignable controller uses the other pair")
{
   down_fixture f;
   f.send({0x40302109u, 0xFFFFFFFFu});     // bank 0x21, index 9, full

   REQUIRE(f._rec._log.size() == 4);
   CHECK(f._rec._log[0].a == 99);  CHECK(f._rec._log[0].b == 0x21);
   CHECK(f._rec._log[1].a == 98);  CHECK(f._rec._log[1].b == 0x09);
   CHECK(f._rec._log[2].a == 6);   CHECK(f._rec._log[2].b == 127);
   CHECK(f._rec._log[3].a == 38);  CHECK(f._rec._log[3].b == 127);
}

TEST_CASE("D.2.4 Program change with a bank is three messages, in order")
{
   // "Bank Select MSB, Bank Select LSB, Program Change"
   down_fixture f;
   f.send({0x40C00001u, 0x05000102u});

   REQUIRE(f._rec._log.size() == 3);
   CHECK(f._rec._log[0].kind == "cc");      CHECK(f._rec._log[0].a == 0);
   CHECK(f._rec._log[0].b == 1);
   CHECK(f._rec._log[1].kind == "cc");      CHECK(f._rec._log[1].a == 32);
   CHECK(f._rec._log[1].b == 2);
   CHECK(f._rec._log[2].kind == "program"); CHECK(f._rec._log[2].a == 5);
}

TEST_CASE("D.2.4 Program change without a bank is one message")
{
   down_fixture f;
   f.send({0x40C00000u, 0x05000000u});

   REQUIRE(f._rec._log.size() == 1);
   CHECK(f._rec._log[0].kind == "program");
}

TEST_CASE("D.2.5 Pitch bend scales 32 to 14, centre to centre")
{
   down_fixture f;
   f.send({0x40E00000u, 0x80000000u});
   f.send({0x40E00000u, 0xFFFFFFFFu});
   f.send({0x40E00000u, 0x00000000u});

   REQUIRE(f._rec._log.size() == 3);
   CHECK(f._rec._log[0].a == 8192);
   CHECK(f._rec._log[1].a == 16383);
   CHECK(f._rec._log[2].a == 0);
}

TEST_CASE("D.2.6 A system message passes as it is")
{
   down_fixture f;
   f.send({0x10F80000u});

   REQUIRE(f._rec._log.size() == 1);
   CHECK(f._rec._log[0].kind == "tick");
}

TEST_CASE("D.2.8 Messages with no MIDI 1.0 equivalent are dropped")
{
   // "Relative Registered Controllers, Relative Assignable Controllers,
   // Per-Note Controllers, Per-Note Management, Per-Note Pitch Bend"
   down_fixture f;
   f.send({0x40400000u, 0x00000001u});     // relative registered
   f.send({0x40500000u, 0x00000001u});     // relative assignable
   f.send({0x40003C4Au, 0x00000001u});     // per-note controller
   f.send({0x40F03C03u, 0x00000000u});     // per-note management
   f.send({0x40603C00u, 0x80000000u});     // per-note pitch bend

   CHECK(f._rec._log.empty());
}

TEST_CASE("A MIDI 1.0 voice message inside a MIDI 2.0 stream passes through")
{
   down_fixture f;
   f.send({0x20913C40u});                  // type 0x2, note on

   REQUIRE(f._rec._log.size() == 1);
   CHECK(f._rec._log[0].kind == "note_on");
   CHECK(f._rec._log[0].b == 64);
}

// D.3 MIDI 1.0 to MIDI 2.0 ///////////////////////////////////////////////////

TEST_CASE("D.3.1 Note on and off scale 7 to 16 with a zero attribute")
{
   // "the Attribute Type shall be set to 0x00 and the Attribute Value
   // shall be set to 0x0000"
   up_fixture f;
   f.send(midi::note_on{1, 60, 127});
   f.send(midi::note_off{1, 60, 64});

   REQUIRE(f._rec._log.size() == 2);
   CHECK(f._rec._log[0].kind == "note_on");
   CHECK(f._rec._log[0].w0 == 0x40913C00u);
   CHECK(f._rec._log[0].w1 == 0xFFFF0000u);
   CHECK(f._rec._log[1].kind == "note_off");
   CHECK(f._rec._log[1].w0 == 0x40813C00u);
   CHECK(f._rec._log[1].w1 == 0x80000000u);
}

TEST_CASE("D.3.1 A note on with velocity zero becomes a note off")
{
   // "shall be translated to a MIDI 2.0 Protocol Note Off message with
   // Velocity 0x0000"
   up_fixture f;
   f.send(midi::note_on{1, 60, 0});

   REQUIRE(f._rec._log.size() == 1);
   CHECK(f._rec._log[0].kind == "note_off");
   CHECK(f._rec._log[0].w1 == 0);
}

TEST_CASE("D.3.2 and D.3.5 Pressure scales 7 to 32")
{
   up_fixture f;
   f.send(midi::poly_aftertouch{0, 60, 64});
   f.send(midi::channel_aftertouch{0, 127});

   REQUIRE(f._rec._log.size() == 2);
   CHECK(f._rec._log[0].kind == "poly");
   CHECK(f._rec._log[0].w1 == 0x80000000u);
   CHECK(f._rec._log[1].kind == "pressure");
   CHECK(f._rec._log[1].w1 == 0xFFFFFFFFu);
}

TEST_CASE("D.3.3 An ordinary control change scales 7 to 32")
{
   up_fixture f;
   f.cc(0, 7, 70);

   REQUIRE(f._rec._log.size() == 1);
   CHECK(f._rec._log[0].kind == "cc");
   CHECK(f._rec._log[0].w0 == 0x40B00700u);
   CHECK(f._rec._log[0].w1 == 0x8C30C30Cu);       // Table 6
}

TEST_CASE("D.3.3 A parameter is held until the fine data entry arrives")
{
   // "The Default Translation shall hold the latest values for controllers
   // CC 6, 98, 99, 100, and 101 until a CC#38 is received."
   up_fixture f;
   f.cc(0, 101, 0);
   f.cc(0, 100, 0);
   f.cc(0, 6, 12);
   CHECK(f._rec._log.empty());

   f.cc(0, 38, 0);
   REQUIRE(f._rec._log.size() == 1);
   CHECK(f._rec._log[0].kind == "rpn");
   CHECK(f._rec._log[0].w0 == 0x40200000u);
   CHECK(f._rec._log[0].w1 == 0x18000000u);       // 1536 << 18, zero extended
}

TEST_CASE("D.3.3 An assignable parameter with an index above 31 stretches")
{
   // M2-115 3.1: min-centre-max for every assignable controller.
   up_fixture f;
   f.cc(0, 99, 0x21);
   f.cc(0, 98, 0x09);
   f.cc(0, 6, 127);
   f.cc(0, 38, 127);

   REQUIRE(f._rec._log.size() == 1);
   CHECK(f._rec._log[0].kind == "nrpn");
   CHECK(f._rec._log[0].w0 == 0x40302109u);
   CHECK(f._rec._log[0].w1 == 0xFFFFFFFFu);
}

TEST_CASE("D.3.3 Increment and decrement become plain control changes")
{
   // "MIDI 1.0 Protocol Inc/Dec messages are translated to Control Change
   // messages in the MIDI 2.0 Protocol."
   up_fixture f;
   f.cc(0, 96, 0);
   f.cc(0, 97, 0);

   REQUIRE(f._rec._log.size() == 2);
   CHECK(f._rec._log[0].kind == "cc");
   CHECK(f._rec._log[0].w0 == 0x40B06000u);
   CHECK(f._rec._log[1].w0 == 0x40B06100u);
}

TEST_CASE("D.3.3 Bank select alone does not translate")
{
   // "Individual use of controllers CC 0 and CC 32 shall not translate"
   up_fixture f;
   f.cc(0, 0, 1);
   f.cc(0, 32, 2);

   CHECK(f._rec._log.empty());
}

TEST_CASE("D.3.4 A program change carries the bank it was given")
{
   up_fixture f;
   f.cc(0, 0, 1);
   f.cc(0, 32, 2);
   f.send(midi::program_change{0, 5});

   REQUIRE(f._rec._log.size() == 1);
   CHECK(f._rec._log[0].kind == "program");
   CHECK(f._rec._log[0].w0 == 0x40C00001u);       // bank valid
   CHECK(f._rec._log[0].w1 == 0x05000102u);
}

TEST_CASE("D.3.4 A program change with no bank known says so")
{
   // "set the Bank Valid bit to 0 and fill the Bank Select fields with
   // zeroes"
   up_fixture f;
   f.send(midi::program_change{0, 5});

   REQUIRE(f._rec._log.size() == 1);
   CHECK(f._rec._log[0].w0 == 0x40C00000u);
   CHECK(f._rec._log[0].w1 == 0x05000000u);
}

TEST_CASE("D.3.6 Pitch bend scales 14 to 32")
{
   up_fixture f;
   f.send(midi::pitch_bend{0, std::uint16_t(8192)});
   f.send(midi::pitch_bend{0, std::uint16_t(16383)});

   REQUIRE(f._rec._log.size() == 2);
   CHECK(f._rec._log[0].w0 == 0x40E00000u);
   CHECK(f._rec._log[0].w1 == 0x80000000u);
   CHECK(f._rec._log[1].w1 == 0xFFFFFFFFu);
}

TEST_CASE("D.3.7 A system message passes as it is")
{
   up_fixture f;
   f.send(midi::timing_tick{});

   REQUIRE(f._rec._log.size() == 1);
   CHECK(f._rec._log[0].kind == "tick");
}

TEST_CASE("D.1.2 Up and back yields the original for every value")
{
   // "The translation algorithm shall yield the same output as the input
   // data when translating MIDI 1.0 -> MIDI 2.0 -> MIDI 1.0"
   for (std::uint32_t v = 0; v != 128; ++v)
   {
      midi1_recorder back;
      midi2::to_midi1<midi1_recorder&> down{back};
      midi2::to_midi2<decltype(down)&> up{down};

      midi::control_change const m{0, midi::cc::controller(7), std::uint8_t(v)};
      midi::raw_message const raw{
         std::uint32_t(m.data[0])
       | (std::uint32_t(m.data[1]) << 8)
       | (std::uint32_t(m.data[2]) << 16)};
      midi::dispatch(raw, 0, up);

      REQUIRE(back._log.size() == 1);
      CHECK(back._log[0].b == v);
   }
}
