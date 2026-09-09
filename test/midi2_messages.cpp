/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Layouts from M2-104-UM, Table 17 (type 0x2), Table 16 (type 0x1) and
// Table 19 (type 0x4). Each packet below is spelled out byte by byte in
// the comment, as the table shows it.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/ump_processor.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;

namespace
{
   // Records what came out, for both protocols at once: a MIDI 2.0 stream
   // may carry MIDI 1.0 voice messages in it, and a processor written for
   // 1.0 must still hear those.
   struct recorder : midi2::processor
   {
      using midi2::processor::operator();

      std::vector<std::string>   _seen;
      std::vector<std::uint32_t> _values;
      std::uint8_t               _group = 0xFF;
      std::uint8_t               _channel = 0xFF;
      std::uint8_t               _key = 0xFF;

      void operator()(midi2::note_on msg, std::size_t)
      {
         _seen.push_back("note_on");
         _group = msg.group();
         _channel = msg.channel();
         _key = msg.key();
         _values.push_back(msg.velocity());
         _values.push_back(msg.attribute_type());
         _values.push_back(msg.attribute());
      }

      void operator()(midi2::note_off msg, std::size_t)
      {
         _seen.push_back("note_off");
         _key = msg.key();
         _values.push_back(msg.velocity());
      }

      void operator()(midi2::poly_pressure msg, std::size_t)
      {
         _seen.push_back("poly_pressure");
         _key = msg.key();
         _values.push_back(msg.value());
      }

      void operator()(midi2::control_change msg, std::size_t)
      {
         _seen.push_back("control_change");
         _values.push_back(msg.controller());
         _values.push_back(msg.value());
      }

      void operator()(midi2::registered_controller msg, std::size_t)
      {
         _seen.push_back("registered_controller");
         _values.push_back(msg.bank());
         _values.push_back(msg.index());
         _values.push_back(msg.value());
      }

      void operator()(midi2::assignable_controller msg, std::size_t)
      {
         _seen.push_back("assignable_controller");
         _values.push_back(msg.bank());
         _values.push_back(msg.index());
         _values.push_back(msg.value());
      }

      void operator()(midi2::relative_registered_controller msg, std::size_t)
      {
         _seen.push_back("relative_registered_controller");
         _values.push_back(std::uint32_t(msg.value()));
      }

      void operator()(midi2::program_change msg, std::size_t)
      {
         _seen.push_back("program_change");
         _values.push_back(msg.program());
         _values.push_back(msg.bank_valid());
         _values.push_back(msg.bank_msb());
         _values.push_back(msg.bank_lsb());
      }

      void operator()(midi2::channel_pressure msg, std::size_t)
      {
         _seen.push_back("channel_pressure");
         _values.push_back(msg.value());
      }

      void operator()(midi2::pitch_bend msg, std::size_t)
      {
         _seen.push_back("pitch_bend");
         _values.push_back(msg.value());
      }

      void operator()(midi2::per_note_pitch_bend msg, std::size_t)
      {
         _seen.push_back("per_note_pitch_bend");
         _key = msg.key();
         _values.push_back(msg.value());
      }

      void operator()(midi2::registered_per_note_controller msg, std::size_t)
      {
         _seen.push_back("registered_per_note_controller");
         _key = msg.key();
         _values.push_back(msg.index());
         _values.push_back(msg.value());
      }

      void operator()(midi2::per_note_management msg, std::size_t)
      {
         _seen.push_back("per_note_management");
         _key = msg.key();
         _values.push_back(msg.detach());
         _values.push_back(msg.reset());
      }

      // MIDI 1.0 messages, as carried in a MIDI 2.0 stream.
      void operator()(midi::note_on msg, std::size_t)
      {
         _seen.push_back("midi1 note_on");
         _channel = msg.channel();
         _key = msg.key();
         _values.push_back(msg.velocity());
      }

      void operator()(midi::pitch_bend msg, std::size_t)
      {
         _seen.push_back("midi1 pitch_bend");
         _values.push_back(msg.value());
      }

      void operator()(midi::timing_tick, std::size_t)
      {
         _seen.push_back("timing_tick");
      }

      void operator()(midi::song_position msg, std::size_t)
      {
         _seen.push_back("song_position");
         _values.push_back(msg.position());
      }
   };

   struct fixture
   {
      void send(midi2::packet const& p)
      {
         midi2::dispatch(p, _time++, _rec);
      }

      recorder     _rec;
      std::size_t  _time = 0;
   };
}

// Type 0x4: MIDI 2.0 channel voice /////////////////////////////////////////

TEST_CASE("Table 19 Note On: 0x4 gggg 1001nnnn rkkkkkkk type | vvvv aaaa")
{
   fixture f;
   // group 3, channel 2, key 60, attribute type 3 (pitch 7.9),
   // velocity 0xFFFF, attribute 0x1234
   f.send({0x43923C03u, 0xFFFF1234u});

   REQUIRE(f._rec._seen == std::vector<std::string>{"note_on"});
   CHECK(f._rec._group == 3);
   CHECK(f._rec._channel == 2);
   CHECK(f._rec._key == 60);
   CHECK(f._rec._values == std::vector<std::uint32_t>{0xFFFF, 3, 0x1234});
}

TEST_CASE("Table 19 Note Off carries its own velocity")
{
   fixture f;
   f.send({0x40803C00u, 0x80000000u});

   REQUIRE(f._rec._seen == std::vector<std::string>{"note_off"});
   CHECK(f._rec._key == 60);
   CHECK(f._rec._values.front() == 0x8000);
}

TEST_CASE("4.2.2 A MIDI 2.0 note on with velocity zero is still a note on")
{
   // "Unlike the MIDI 1.0 Note On message, a velocity value of zero does
   // not function as a Note Off."
   fixture f;
   f.send({0x40903C00u, 0x00000000u});

   CHECK(f._rec._seen == std::vector<std::string>{"note_on"});
}

TEST_CASE("Table 19 Poly Pressure: 32 bit value")
{
   fixture f;
   f.send({0x40A03C00u, 0xDEADBEEFu});

   REQUIRE(f._rec._seen == std::vector<std::string>{"poly_pressure"});
   CHECK(f._rec._key == 60);
   CHECK(f._rec._values.front() == 0xDEADBEEF);
}

TEST_CASE("Table 19 Control Change: 0x4 gggg 1011nnnn rccccccc reserved")
{
   fixture f;
   f.send({0x40B00700u, 0x80000000u});      // controller 7, half

   REQUIRE(f._rec._seen == std::vector<std::string>{"control_change"});
   CHECK(f._rec._values == std::vector<std::uint32_t>{7, 0x80000000u});
}

TEST_CASE("Table 19 Registered Controller: bank and index, one message")
{
   // 4.2.7: what MIDI 1.0 needs four controllers to say. Bank 0, index 0
   // is pitch bend sensitivity.
   fixture f;
   f.send({0x40200000u, 0x0C000000u});

   REQUIRE(f._rec._seen == std::vector<std::string>{"registered_controller"});
   CHECK(f._rec._values == std::vector<std::uint32_t>{0, 0, 0x0C000000u});
}

TEST_CASE("Table 19 Assignable Controller: bank 0x21, index 0x09")
{
   fixture f;
   f.send({0x40302109u, 0x00000001u});

   REQUIRE(f._rec._seen == std::vector<std::string>{"assignable_controller"});
   CHECK(f._rec._values == std::vector<std::uint32_t>{0x21, 0x09, 1});
}

TEST_CASE("4.2.8 A relative controller's data is two's complement")
{
   fixture f;
   f.send({0x40400000u, 0xFFFFFFFFu});      // minus one

   REQUIRE(f._rec._seen ==
      std::vector<std::string>{"relative_registered_controller"});
   CHECK(std::int32_t(f._rec._values.front()) == -1);
}

TEST_CASE("Table 19 Program Change: program, and a bank when B is set")
{
   // 4.2.9: "If the Sender sets the Bank Valid bit to 1, then the Receiver
   // performs first the Bank Select operation and then the Program Change"
   fixture f;
   f.send({0x40C00001u, 0x05000102u});      // B=1, program 5, bank 1/2

   REQUIRE(f._rec._seen == std::vector<std::string>{"program_change"});
   CHECK(f._rec._values == std::vector<std::uint32_t>{5, 1, 1, 2});
}

TEST_CASE("Table 19 Program Change without a bank")
{
   fixture f;
   f.send({0x40C00000u, 0x05000000u});

   CHECK(f._rec._values == std::vector<std::uint32_t>{5, 0, 0, 0});
}

TEST_CASE("Table 19 Channel Pressure and Pitch Bend: 32 bit data")
{
   fixture f;
   f.send({0x40D00000u, 0x40000000u});
   f.send({0x40E00000u, 0x80000000u});      // 4.2.11: centred here

   REQUIRE(f._rec._seen ==
      std::vector<std::string>{"channel_pressure", "pitch_bend"});
   CHECK(f._rec._values ==
      std::vector<std::uint32_t>{0x40000000u, 0x80000000u});
}

TEST_CASE("Table 19 Per-Note Pitch Bend names the note it bends")
{
   // 4.2.12: "acts like Pitch Bend in every way, except that it applies to
   // individual Note Numbers"
   fixture f;
   f.send({0x40603C00u, 0xC0000000u});

   REQUIRE(f._rec._seen == std::vector<std::string>{"per_note_pitch_bend"});
   CHECK(f._rec._key == 60);
   CHECK(f._rec._values.front() == 0xC0000000u);
}

TEST_CASE("Table 19 Registered Per-Note Controller: note, index, data")
{
   fixture f;
   f.send({0x40003C4Au, 0x12345678u});      // key 60, index 74

   REQUIRE(f._rec._seen ==
      std::vector<std::string>{"registered_per_note_controller"});
   CHECK(f._rec._key == 60);
   CHECK(f._rec._values == std::vector<std::uint32_t>{74, 0x12345678u});
}

TEST_CASE("4.2.5 Per-Note Management: D and S flags")
{
   fixture f;
   f.send({0x40F03C03u, 0u});               // both set

   REQUIRE(f._rec._seen == std::vector<std::string>{"per_note_management"});
   CHECK(f._rec._key == 60);
   CHECK(f._rec._values == std::vector<std::uint32_t>{1, 1});
}

// Type 0x2: MIDI 1.0 channel voice, inside a packet ////////////////////////

TEST_CASE("Table 17 A MIDI 1.0 note on in a packet reaches a 1.0 processor")
{
   // 0x2 gggg 1001nnnn rkkkkkkk rvvvvvvv
   fixture f;
   f.send({0x20913C40u});

   REQUIRE(f._rec._seen == std::vector<std::string>{"midi1 note_on"});
   CHECK(f._rec._channel == 1);
   CHECK(f._rec._key == 60);
   CHECK(f._rec._values.front() == 64);
}

TEST_CASE("Table 17 MIDI 1.0 pitch bend: lsb then msb")
{
   // 0x2 gggg 1110nnnn rddddddd rDDDDDDD, low half first.
   fixture f;
   f.send({0x20E00040u});                   // lsb 0, msb 64: centre

   REQUIRE(f._rec._seen == std::vector<std::string>{"midi1 pitch_bend"});
   CHECK(f._rec._values.front() == 8192);
}

// Type 0x1: system ///////////////////////////////////////////////////////////

TEST_CASE("Table 16 Timing clock is a single status byte")
{
   fixture f;
   f.send({0x10F80000u});

   CHECK(f._rec._seen == std::vector<std::string>{"timing_tick"});
}

TEST_CASE("Table 16 Song position: 0lllllll 0mmmmmmm")
{
   fixture f;
   f.send({0x10F20110u});                   // lsb 1, msb 16

   REQUIRE(f._rec._seen == std::vector<std::string>{"song_position"});
   CHECK(f._rec._values.front() == (1 | (16 << 7)));
}

// The rest ///////////////////////////////////////////////////////////////////

TEST_CASE("2.1.3 A reserved type dispatches nothing")
{
   fixture f;
   f.send({0x60000000u});
   f.send({0xB0000000u, 0u, 0u});

   CHECK(f._rec._seen.empty());
}

TEST_CASE("Utility messages dispatch nothing yet")
{
   // NOOP and the timestamps are transport concerns; a processor hears
   // no message from them.
   fixture f;
   f.send({0x00000000u});

   CHECK(f._rec._seen.empty());
}
