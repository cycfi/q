/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// Building the MIDI 2.0 channel voice messages from their fields, which is
// what a synth, a sequencer or a controller does. Read from M2-104-UM
// version 1.0: Table 19 for all fifteen layouts, Figure 21 for the per-note
// management option flags, Figure 27 for the program change bank valid bit,
// and Table 5 for the attribute types. Every packet a builder makes is
// checked three ways: its words against the layout, its fields read back
// through the accessors the builder is the inverse of, and, where MIDI 1.0
// has the same message, against what to_midi2 produces from it.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/ump_messages.hpp>
#include <q/midi/translate.hpp>

#include <cstdint>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;

namespace
{
   // Both words of whatever the translator hands on.
   struct words : midi2::processor
   {
      using midi2::processor::operator();

                              template <typename Message>
      void                    operator()(Message msg, std::size_t)
                              requires (Message::words == 2)
                              {
                                 _w0 = msg.word(0);
                                 _w1 = msg.word(1);
                                 ++_count;
                              }

      std::uint32_t  _w0 = 0;
      std::uint32_t  _w1 = 0;
      int            _count = 0;
   };
}

TEST_CASE("Table 19: a note on is built word for word")
{
   // Type 0x4, group 3, opcode 0x9, channel 5, note 60, attribute type 0;
   // then 16 bit velocity and the attribute.
   midi2::note_on const msg{3, 5, 60, 0x1234};

   CHECK(msg.word(0) == 0x43953C00u);
   CHECK(msg.word(1) == 0x12340000u);
}

TEST_CASE("4.2.2: a note on carries an attribute when given one")
{
   // Attribute type 3 is Pitch 7.9, which is what puts an exact pitch in
   // the note on itself rather than in a bend that follows it.
   midi2::note_on const msg{3, 5, 60, 0x1234, 3, 0x0800};

   CHECK(msg.word(0) == 0x43953C03u);
   CHECK(msg.word(1) == 0x12340800u);
   CHECK(msg.attribute_type() == 3);
   CHECK(msg.attribute() == 0x0800);
}

TEST_CASE("Table 19: the other layouts, word for word")
{
   SECTION("control change, 4.2.6")
   {
      midi2::control_change const msg{0, 0, 74, 0x80000000u};
      CHECK(msg.word(0) == 0x40B04A00u);
      CHECK(msg.word(1) == 0x80000000u);
   }

   SECTION("registered controller, 4.2.7")
   {
      // Bank 0 index 6 is pitch bend sensitivity, 12 semitones.
      midi2::registered_controller const msg{1, 2, 0, 6, 0x18000000u};
      CHECK(msg.word(0) == 0x41220006u);
      CHECK(msg.word(1) == 0x18000000u);
   }

   SECTION("per note pitch bend, 4.2.12")
   {
      midi2::per_note_pitch_bend const msg{
         0, 0, 60, midi2::pitch_bend::center};
      CHECK(msg.word(0) == 0x40603C00u);
      CHECK(msg.word(1) == 0x80000000u);
   }

   SECTION("per note management, 4.2.5")
   {
      midi2::per_note_management const msg{0, 0, 60, true, true};
      CHECK(msg.word(0) == 0x40F03C03u);
      CHECK(msg.word(1) == 0u);
   }

   SECTION("pitch bend, 4.2.11")
   {
      midi2::pitch_bend const msg{0, 0, midi2::pitch_bend::center};
      CHECK(msg.word(0) == 0x40E00000u);
      CHECK(msg.word(1) == 0x80000000u);
   }

   SECTION("program change with a bank, 4.2.9")
   {
      // The bank is one number of 16384, split into halves on the wire.
      midi2::program_change const msg{0, 0, 5, std::uint16_t(130)};
      CHECK(msg.word(0) == 0x40C00001u);
      CHECK(msg.word(1) == 0x05000102u);
      CHECK(msg.bank() == 130);
   }

   SECTION("the two halves build the same message as the whole bank")
   {
      midi2::program_change const whole{0, 0, 5, std::uint16_t(16383)};
      midi2::program_change const halves{0, 0, 5, 0x7F, 0x7F};
      CHECK(whole.word(1) == halves.word(1));
      CHECK(whole.word(1) == 0x05007F7Fu);
      CHECK(whole.bank() == 16383);
   }

   SECTION("program change without one leaves the bank invalid")
   {
      // 4.2.9: with the bank valid bit clear "the Sender shall also fill
      // the Bank MSB and Bank LSB fields with zeroes".
      midi2::program_change const msg{0, 0, 5};
      CHECK(msg.word(0) == 0x40C00000u);
      CHECK(msg.word(1) == 0x05000000u);
      CHECK(!msg.bank_valid());
      CHECK(msg.bank() == 0);
      CHECK(msg.bank_msb() == 0);
      CHECK(msg.bank_lsb() == 0);
   }
}

TEST_CASE("Every voice message reads back what it was built from")
{
   SECTION("note off")
   {
      midi2::note_off const msg{1, 2, 64, 0xFFFF, 1, 0x0040};
      CHECK(msg.message_type() == 0x4);
      CHECK(msg.group() == 1);
      CHECK(msg.opcode() == midi2::opcode::note_off);
      CHECK(msg.channel() == 2);
      CHECK(msg.key() == 64);
      CHECK(msg.velocity() == 0xFFFF);
      CHECK(msg.attribute_type() == 1);
      CHECK(msg.attribute() == 0x0040);
   }

   SECTION("poly pressure")
   {
      midi2::poly_pressure const msg{2, 3, 60, 0xDEADBEEFu};
      CHECK(msg.opcode() == midi2::opcode::poly_pressure);
      CHECK(msg.group() == 2);
      CHECK(msg.channel() == 3);
      CHECK(msg.key() == 60);
      CHECK(msg.value() == 0xDEADBEEFu);
   }

   SECTION("registered per note controller")
   {
      midi2::registered_per_note_controller const msg{
         0, 1, 60, 74, 0x40000000u};
      CHECK(msg.opcode() == midi2::opcode::registered_per_note);
      CHECK(msg.key() == 60);
      CHECK(msg.index() == 74);
      CHECK(msg.value() == 0x40000000u);
   }

   SECTION("assignable per note controller")
   {
      midi2::assignable_per_note_controller const msg{
         0, 1, 60, 7, 0x20000000u};
      CHECK(msg.opcode() == midi2::opcode::assignable_per_note);
      CHECK(msg.index() == 7);
      CHECK(msg.value() == 0x20000000u);
   }

   SECTION("per note management")
   {
      midi2::per_note_management const detach_only{0, 0, 60, true, false};
      CHECK(detach_only.opcode() == midi2::opcode::per_note_management);
      CHECK(detach_only.detach());
      CHECK(!detach_only.reset());

      midi2::per_note_management const reset_only{0, 0, 60, false, true};
      CHECK(!reset_only.detach());
      CHECK(reset_only.reset());
   }

   SECTION("assignable controller")
   {
      midi2::assignable_controller const msg{0, 4, 3, 9, 0x01020304u};
      CHECK(msg.opcode() == midi2::opcode::assignable);
      CHECK(msg.channel() == 4);
      CHECK(msg.bank() == 3);
      CHECK(msg.index() == 9);
      CHECK(msg.value() == 0x01020304u);
   }

   SECTION("the relative forms carry a signed value")
   {
      midi2::relative_registered_controller const up{0, 0, 0, 6, 1000};
      CHECK(up.opcode() == midi2::opcode::relative_registered);
      CHECK(up.value() == 1000);

      midi2::relative_assignable_controller const down{0, 0, 0, 6, -1000};
      CHECK(down.opcode() == midi2::opcode::relative_assignable);
      CHECK(down.value() == -1000);
   }

   SECTION("channel pressure")
   {
      midi2::channel_pressure const msg{5, 6, 0x12345678u};
      CHECK(msg.opcode() == midi2::opcode::channel_pressure);
      CHECK(msg.group() == 5);
      CHECK(msg.channel() == 6);
      CHECK(msg.value() == 0x12345678u);
   }

   SECTION("program change")
   {
      midi2::program_change const msg{0, 0, 127, std::uint16_t(130)};
      CHECK(msg.opcode() == midi2::opcode::program_change);
      CHECK(msg.bank_valid());
      CHECK(msg.program() == 127);
      CHECK(msg.bank() == 130);
      CHECK(msg.bank_msb() == 1);
      CHECK(msg.bank_lsb() == 2);
   }
}

TEST_CASE("4.2.13: a note built without an attribute says it has none")
{
   // "In a Note On/Off message with no attribute data, the Attribute Type
   // shall be set to 0x00 and the Attribute Data shall be set to 0x0000",
   // which is what the default arguments give.
   midi2::note_on const on{0, 0, 60, 0x1234};
   CHECK(on.attribute_type() == 0);
   CHECK(on.attribute() == 0);

   midi2::note_off const off{0, 0, 60, 0x1234};
   CHECK(off.attribute_type() == 0);
   CHECK(off.attribute() == 0);
}

TEST_CASE("Group and channel are held to four bits")
{
   // A caller that oversteps must not spill into the fields beside it.
   midi2::note_on const msg{0xFF, 0xFF, 60, 0x1234};

   CHECK(msg.message_type() == 0x4);
   CHECK(msg.group() == 0xF);
   CHECK(msg.channel() == 0xF);
   CHECK(msg.key() == 60);
}

TEST_CASE("A built message is what the translator makes of the 1.0 one")
{
   // to_midi2 was written and tested against the specification on its own,
   // so agreeing with it is a check from outside these builders. Every
   // translated packet goes to group 0, and the values are scaled.
   words out;
   midi2::to_midi2 translate{std::ref(out)};

   SECTION("note on")
   {
      translate(midi::note_on{5, 60, 100}, 0);
      REQUIRE(out._count == 1);

      midi2::note_on const built{
         0, 5, 60, std::uint16_t(out._w1 >> 16)};
      CHECK(built.word(0) == out._w0);
      CHECK(built.word(1) == out._w1);
   }

   SECTION("control change")
   {
      translate(midi::control_change{2, midi::cc::controller(74), 64}, 0);
      REQUIRE(out._count == 1);

      midi2::control_change const built{0, 2, 74, out._w1};
      CHECK(built.word(0) == out._w0);
      CHECK(built.word(1) == out._w1);
   }

   SECTION("channel pressure")
   {
      translate(midi::channel_pressure{3, 90}, 0);
      REQUIRE(out._count == 1);

      midi2::channel_pressure const built{0, 3, out._w1};
      CHECK(built.word(0) == out._w0);
      CHECK(built.word(1) == out._w1);
   }

   SECTION("pitch bend")
   {
      translate(midi::pitch_bend{7, 0x2000}, 0);
      REQUIRE(out._count == 1);

      midi2::pitch_bend const built{0, 7, out._w1};
      CHECK(built.word(0) == out._w0);
      CHECK(built.word(1) == out._w1);
   }
}
