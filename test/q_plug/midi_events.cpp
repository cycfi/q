/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The host's note events, in each dialect CLAP offers, turned into the Q
// messages a processor handles. Cases are named for the clauses of
// clap/events.h and clap/ext/note-ports.h they come from.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <qplug/clap/midi_events.hpp>
#include <qplug/midi_processor.hpp>
#include <q/midi/packet_writer.hpp>
#include <q/midi/scaling.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;
namespace qplug = cycfi::qplug;

namespace
{
   clap_event_midi_t midi_event(
      std::uint8_t status, std::uint8_t d1, std::uint8_t d2
    , std::uint32_t time = 0)
   {
      clap_event_midi_t ev{};
      ev.header.size = sizeof(ev);
      ev.header.time = time;
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.type = CLAP_EVENT_MIDI;
      ev.data[0] = status;
      ev.data[1] = d1;
      ev.data[2] = d2;
      return ev;
   }

   clap_event_note_t note_event(
      std::uint16_t type, std::int16_t channel, std::int16_t key
    , double velocity, std::int32_t note_id = -1)
   {
      clap_event_note_t ev{};
      ev.header.size = sizeof(ev);
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.type = type;
      ev.note_id = note_id;
      ev.port_index = 0;
      ev.channel = channel;
      ev.key = key;
      ev.velocity = velocity;
      return ev;
   }

   clap_event_note_expression_t expression_event(
      clap_note_expression id, std::int16_t channel, std::int16_t key
    , double value)
   {
      clap_event_note_expression_t ev{};
      ev.header.size = sizeof(ev);
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.type = CLAP_EVENT_NOTE_EXPRESSION;
      ev.expression_id = id;
      ev.note_id = -1;
      ev.port_index = 0;
      ev.channel = channel;
      ev.key = key;
      ev.value = value;
      return ev;
   }

   clap_event_midi2_t midi2_event(std::uint32_t w0, std::uint32_t w1 = 0)
   {
      clap_event_midi2_t ev{};
      ev.header.size = sizeof(ev);
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.type = CLAP_EVENT_MIDI2;
      ev.data[0] = w0;
      ev.data[1] = w1;
      return ev;
   }

   midi2::packet packet_of(clap_event_note_t const& ev)
   {
      midi2::packet p{0};
      REQUIRE(qplug::to_packet(ev, p));
      return p;
   }

   midi2::packet packet_of(clap_event_note_expression_t const& ev)
   {
      midi2::packet p{0};
      REQUIRE(qplug::to_packet(ev, p));
      return p;
   }

   struct note
   {
      std::uint8_t   channel;
      std::uint8_t   key;
      std::uint32_t  velocity;
      std::size_t    time;
   };

   struct per_note
   {
      std::uint8_t   key;
      std::uint8_t   index;
      std::uint32_t  value;
   };

   // A processor that writes down what reached it, through the same base
   // an example would use: MIDI 2.0 overloads, the default.
   struct recorder : qplug::midi_processor<recorder>
   {
      using midi_processor::operator();

      void operator()(midi2::note_on msg, std::size_t time)
      {
         _on.push_back({msg.channel(), msg.key(), msg.velocity(), time});
      }

      void operator()(midi2::note_off msg, std::size_t time)
      {
         _off.push_back({msg.channel(), msg.key(), msg.velocity(), time});
      }

      void operator()(midi::note_on msg, std::size_t time)
      {
         _on1.push_back({msg.channel(), msg.key(), msg.velocity(), time});
      }

      void operator()(midi2::control_change msg, std::size_t time)
      {
         _ccs.push_back(
            {msg.channel(), msg.controller(), msg.value(), time});
      }

      void operator()(midi2::per_note_pitch_bend msg, std::size_t time)
      {
         _bends.push_back({msg.channel(), msg.key(), msg.value(), time});
      }

      void operator()(midi2::registered_per_note_controller msg, std::size_t)
      {
         _per_note.push_back({msg.key(), msg.index(), msg.value()});
      }

      void operator()(midi2::poly_pressure msg, std::size_t)
      {
         _pressure.push_back({msg.key(), 0, msg.value()});
      }

      void operator()(midi::sysex_view msg, std::size_t)
      {
         _sysex.assign(msg.data().begin(), msg.data().end());
      }

      std::vector<note>          _on;
      std::vector<note>          _off;
      std::vector<note>          _on1;
      std::vector<note>          _ccs;
      std::vector<note>          _bends;
      std::vector<per_note>      _per_note;
      std::vector<per_note>      _pressure;
      std::vector<std::uint8_t>  _sysex;

      // The framework hands MIDI to a processor through this base, never
      // to the derived type, so the tests do the same.
      qplug::processor& base() { return *this; }
   };

   // The same, written for MIDI 1.0: the best effort direction.
   struct recorder1 : qplug::midi_processor<recorder1, midi::processor>
   {
      using midi_processor::operator();

      void operator()(midi::note_on msg, std::size_t time)
      {
         _on.push_back({msg.channel(), msg.key(), msg.velocity(), time});
      }

      void operator()(midi::note_off msg, std::size_t time)
      {
         _off.push_back({msg.channel(), msg.key(), msg.velocity(), time});
      }

      void operator()(midi2::note_on msg, std::size_t time)
      {
         _on2.push_back({msg.channel(), msg.key(), msg.velocity(), time});
      }

      void operator()(midi::control_change msg, std::size_t time)
      {
         _ccs.push_back({msg.channel(), std::uint8_t(msg.controller())
                       , msg.value(), time});
      }

      void operator()(midi::poly_aftertouch msg, std::size_t time)
      {
         _pressure.push_back(
            {msg.channel(), msg.key(), msg.pressure(), time});
      }

      void operator()(midi::pitch_bend msg, std::size_t time)
      {
         _bends.push_back({msg.channel(), 0, msg.value(), time});
      }

      std::vector<note>          _on;
      std::vector<note>          _off;
      std::vector<note>          _on2;
      std::vector<note>          _ccs;
      std::vector<note>          _pressure;
      std::vector<note>          _bends;

      qplug::processor& base() { return *this; }
   };
}

////////////////////////////////////////////////////////////////////////////
// The MIDI dialect: clap_event_midi, three bytes, already a MIDI message.
////////////////////////////////////////////////////////////////////////////
TEST_CASE("A MIDI dialect event is the message it already is")
{
   auto const raw = qplug::to_raw_message(midi_event(0x91, 60, 100));
   CHECK((raw.data & 0xFF) == 0x91);
   CHECK(((raw.data >> 8) & 0xFF) == 60);
   CHECK(((raw.data >> 16) & 0xFF) == 100);
}

TEST_CASE("A MIDI dialect event of one or two bytes carries no more")
{
   // 0xD0, channel pressure, is two bytes. The third must not appear.
   auto const raw = qplug::to_raw_message(midi_event(0xD0, 64, 0x7F));
   CHECK((raw.data & 0xFF) == 0xD0);
   CHECK(((raw.data >> 8) & 0xFF) == 64);
   CHECK(((raw.data >> 16) & 0xFF) == 0);
}

TEST_CASE("A MIDI 1.0 note reaches the MIDI 2.0 overloads, scaled up")
{
   // A processor writes MIDI 2.0 overloads and receives every dialect: the
   // byte path runs through Q's to_midi2, so a 7 bit velocity arrives as
   // the 16 bit one MIDI 2.0 carries.
   recorder rec;
   rec.base().midi(qplug::to_raw_message(midi_event(0x91, 60, 100)), 7);

   REQUIRE(rec._on.size() == 1);
   CHECK(rec._on[0].channel == 1);
   CHECK(rec._on[0].key == 60);
   CHECK(rec._on[0].velocity == midi2::scale_up(100, 7, 16));
   CHECK(rec._on[0].time == 7);
   CHECK(rec._on1.empty());
}

TEST_CASE("A MIDI 1.0 note on of zero velocity arrives as a note off")
{
   // D.3.1: MIDI 1.0 means a note off by it, and MIDI 2.0 can say so.
   recorder rec;
   rec.base().midi(qplug::to_raw_message(midi_event(0x90, 60, 0)), 0);

   CHECK(rec._on.empty());
   REQUIRE(rec._off.size() == 1);
   CHECK(rec._off[0].key == 60);
   CHECK(rec._off[0].velocity == 0);
}

////////////////////////////////////////////////////////////////////////////
// The CLAP dialect: clap_event_note, a typed note with a 0..1 velocity,
// and clap_event_note_expression. Both become MIDI 2.0.
////////////////////////////////////////////////////////////////////////////
TEST_CASE("A CLAP note on becomes a MIDI 2.0 note on")
{
   auto const p = packet_of(note_event(CLAP_EVENT_NOTE_ON, 2, 64, 1.0));
   CHECK(p.message_type() == midi2::message_type::midi2_voice);
   CHECK(p.status() == midi2::opcode::note_on);

   recorder rec;
   rec.base().midi(p, 3);

   REQUIRE(rec._on.size() == 1);
   CHECK(rec._on[0].channel == 2);
   CHECK(rec._on[0].key == 64);
   CHECK(rec._on[0].velocity == 0xFFFF);
   CHECK(rec._on[0].time == 3);
}

TEST_CASE("A CLAP note off becomes a MIDI 2.0 note off")
{
   recorder rec;
   auto const p = packet_of(note_event(CLAP_EVENT_NOTE_OFF, 0, 60, 0.0));
   rec.base().midi(p, 0);

   REQUIRE(rec._off.size() == 1);
   CHECK(rec._off[0].key == 60);
   CHECK(rec._off[0].velocity == 0);
}

TEST_CASE("Velocity scales across the range, the ends exact")
{
   struct { double in; std::uint32_t out; } const cases[] =
   {
      {1.0, 0xFFFF}, {0.5, 0x8000}, {0.0, 0}, {2.0, 0xFFFF}, {-1.0, 0}
   };

   for (auto const& c : cases)
   {
      auto const p = packet_of(note_event(CLAP_EVENT_NOTE_ON, 0, 60, c.in));
      CHECK((p.word(1) >> 16) == c.out);
   }
}

TEST_CASE("A CLAP note on of zero velocity is a quiet note on")
{
   // 4.2.2: MIDI 2.0 reads no note off into it, so none is made.
   recorder rec;
   rec.base().midi(packet_of(note_event(CLAP_EVENT_NOTE_ON, 0, 60, 0.0)), 0);

   REQUIRE(rec._on.size() == 1);
   CHECK(rec._on[0].velocity == 0);
   CHECK(rec._off.empty());
}

TEST_CASE("A wildcard addresses voices, not a MIDI message")
{
   // -1 in the tuple is a wildcard, which no MIDI message can say.
   midi2::packet p{0};
   CHECK(!qplug::to_packet(note_event(CLAP_EVENT_NOTE_OFF, -1, 60, 0.0), p));
   CHECK(!qplug::to_packet(note_event(CLAP_EVENT_NOTE_OFF, 0, -1, 0.0), p));
}

TEST_CASE("A choke is a voice instruction, not a note")
{
   midi2::packet p{0};
   CHECK(!qplug::to_packet(
      note_event(CLAP_EVENT_NOTE_CHOKE, 0, 60, 0.0), p));
}

TEST_CASE("A pressure expression becomes poly pressure")
{
   recorder rec;
   rec.base().midi(
      packet_of(expression_event(CLAP_NOTE_EXPRESSION_PRESSURE, 1, 60, 1.0))
    , 0);

   REQUIRE(rec._pressure.size() == 1);
   CHECK(rec._pressure[0].key == 60);
   CHECK(rec._pressure[0].value == 0xFFFFFFFFu);
}

TEST_CASE("A tuning expression becomes the per-note controller Pitch 7.25")
{
   // Key 60 raised a quarter tone: 60.5 semitones, 7 bits of semitone and
   // 25 of fraction.
   recorder rec;
   rec.base().midi(
      packet_of(expression_event(CLAP_NOTE_EXPRESSION_TUNING, 0, 60, 0.5))
    , 0);

   REQUIRE(rec._per_note.size() == 1);
   CHECK(rec._per_note[0].key == 60);
   CHECK(rec._per_note[0].index == 3);
   CHECK(rec._per_note[0].value == ((60u << 25) | (1u << 24)));
}

TEST_CASE("The other expressions become the per-note controllers so named")
{
   struct { clap_note_expression id; std::uint8_t index; } const cases[] =
   {
      {CLAP_NOTE_EXPRESSION_VOLUME, 7}
    , {CLAP_NOTE_EXPRESSION_PAN, 10}
    , {CLAP_NOTE_EXPRESSION_VIBRATO, 77}
    , {CLAP_NOTE_EXPRESSION_EXPRESSION, 11}
    , {CLAP_NOTE_EXPRESSION_BRIGHTNESS, 74}
   };

   for (auto const& c : cases)
   {
      recorder rec;
      rec.base().midi(packet_of(expression_event(c.id, 0, 62, 0.5)), 0);

      REQUIRE(rec._per_note.size() == 1);
      CHECK(rec._per_note[0].key == 62);
      CHECK(rec._per_note[0].index == c.index);
      CHECK(rec._per_note[0].value == 0x80000000u);
   }
}

TEST_CASE("A volume above one, a gain, clamps to full")
{
   auto const p = packet_of(
      expression_event(CLAP_NOTE_EXPRESSION_VOLUME, 0, 60, 4.0));
   CHECK(p.word(1) == 0xFFFFFFFFu);
}

TEST_CASE("An expression with a wildcard is dropped")
{
   midi2::packet p{0};
   CHECK(!qplug::to_packet(
      expression_event(CLAP_NOTE_EXPRESSION_PAN, -1, 60, 0.5), p));
   CHECK(!qplug::to_packet(
      expression_event(CLAP_NOTE_EXPRESSION_PAN, 0, -1, 0.5), p));
}

////////////////////////////////////////////////////////////////////////////
// The MIDI 2.0 dialect: clap_event_midi2, four words, already a packet.
////////////////////////////////////////////////////////////////////////////
TEST_CASE("A MIDI 2.0 event is the packet it already is")
{
   auto const p = qplug::to_packet(midi2_event(0x40903C00u, 0xBEEF0000u));
   CHECK(p.word(0) == 0x40903C00u);
   CHECK(p.word(1) == 0xBEEF0000u);
   CHECK(p.words() == 2);
}

TEST_CASE("A MIDI 2.0 note reaches the MIDI 2.0 overloads whole")
{
   recorder rec;
   auto const p = qplug::to_packet(midi2_event(0x40903C00u, 0xFFFF0000u));
   rec.base().midi(p, 7);

   REQUIRE(rec._on.size() == 1);
   CHECK(rec._on[0].key == 60);
   CHECK(rec._on[0].velocity == 0xFFFF);
   CHECK(rec._on[0].time == 7);
   CHECK(rec._on1.empty());
}

TEST_CASE("A MIDI 2.0 velocity of one arrives as one")
{
   recorder rec;
   auto const p = qplug::to_packet(midi2_event(0x40903C00u, 0x00010000u));
   rec.base().midi(p, 0);

   REQUIRE(rec._on.size() == 1);
   CHECK(rec._on[0].velocity == 1);
}

TEST_CASE("A MIDI 1.0 packet reaches the MIDI 2.0 overloads, scaled up")
{
   // Control change 7, value 100, carried in a packet as MIDI 1.0.
   recorder rec;
   rec.base().midi(qplug::to_packet(midi2_event(0x20B00764u)), 0);

   REQUIRE(rec._ccs.size() == 1);
   CHECK(rec._ccs[0].key == 7);
   CHECK(rec._ccs[0].velocity == midi2::scale_up(100, 7, 32));
}

TEST_CASE("A per-note message arrives")
{
   // Per-note pitch bend, channel 0, key 60, has no MIDI 1.0 form and
   // arrives whole in MIDI 2.0.
   recorder rec;
   auto const p = qplug::to_packet(midi2_event(0x40603C00u, 0x80000000u));
   rec.base().midi(p, 0);

   REQUIRE(rec._bends.size() == 1);
   CHECK(rec._bends[0].key == 60);
   CHECK(rec._bends[0].velocity == 0x80000000u);
}

TEST_CASE("System exclusive spans packets and arrives whole")
{
   recorder rec;
   std::uint8_t const payload[] = {0x7E, 0x7F, 0x0D, 0x70, 0x11, 0x22, 0x33};
   midi2::send_sysex7(payload
    , [&](midi2::packet const& p) { rec.base().midi(p, 0); });

   REQUIRE(rec._sysex.size() == 7);
   CHECK(rec._sysex.front() == 0x7E);
   CHECK(rec._sysex.back() == 0x33);
}

////////////////////////////////////////////////////////////////////////////
// The processor base
////////////////////////////////////////////////////////////////////////////
TEST_CASE("A processor receives only the overloads it writes")
{
   // Nothing here handles pitch bend; it must pass without a trace.
   recorder rec;
   rec.base().midi(qplug::to_raw_message(midi_event(0xE0, 0, 64)), 0);
   CHECK(rec._on.empty());
   CHECK(rec._ccs.empty());
}

TEST_CASE("The sample offset in the block is the message time")
{
   recorder rec;
   rec.base().midi(qplug::to_raw_message(midi_event(0x90, 60, 100, 0)), 0);
   rec.base().midi(qplug::to_raw_message(midi_event(0x90, 62, 100, 0)), 128);

   REQUIRE(rec._on.size() == 2);
   CHECK(rec._on[0].time == 0);
   CHECK(rec._on[1].time == 128);
}

////////////////////////////////////////////////////////////////////////////
// A MIDI 1.0 processor: the best effort direction
////////////////////////////////////////////////////////////////////////////
TEST_CASE("A MIDI 1.0 processor receives MIDI 1.0 bytes as they are")
{
   recorder1 rec;
   rec.base().midi(qplug::to_raw_message(midi_event(0x91, 60, 100)), 7);

   REQUIRE(rec._on.size() == 1);
   CHECK(rec._on[0].channel == 1);
   CHECK(rec._on[0].key == 60);
   CHECK(rec._on[0].velocity == 100);
   CHECK(rec._on[0].time == 7);
}

TEST_CASE("A MIDI 1.0 processor receives a MIDI 2.0 note scaled down")
{
   // The packet path runs through Q's to_midi1, so a 16 bit velocity
   // arrives as the 7 bit one MIDI 1.0 has room for, and the MIDI 2.0
   // overload beside it is never called.
   recorder1 rec;
   auto const p = qplug::to_packet(midi2_event(0x40903C00u, 0xFFFF0000u));
   rec.base().midi(p, 7);

   REQUIRE(rec._on.size() == 1);
   CHECK(rec._on[0].key == 60);
   CHECK(rec._on[0].velocity == 127);
   CHECK(rec._on[0].time == 7);
   CHECK(rec._on2.empty());
}

TEST_CASE("A MIDI 1.0 processor keeps a note on that scales to zero")
{
   // D.2: zero would read as a note off, so the floor is one.
   recorder1 rec;
   auto const p = qplug::to_packet(midi2_event(0x40903C00u, 0x00010000u));
   rec.base().midi(p, 0);

   REQUIRE(rec._on.size() == 1);
   CHECK(rec._on[0].velocity == 1);
}

TEST_CASE("A MIDI 1.0 processor receives a CLAP note scaled down")
{
   // A CLAP note is MIDI 2.0 first, then narrowed like any other. A note
   // on of zero velocity is floored to one on the way.
   recorder1 rec;
   rec.base().midi(packet_of(note_event(CLAP_EVENT_NOTE_ON, 2, 64, 1.0)), 0);
   rec.base().midi(packet_of(note_event(CLAP_EVENT_NOTE_ON, 2, 65, 0.0)), 0);
   rec.base().midi(
      packet_of(note_event(CLAP_EVENT_NOTE_OFF, 2, 64, 0.0)), 0);

   REQUIRE(rec._on.size() == 2);
   CHECK(rec._on[0].key == 64);
   CHECK(rec._on[0].velocity == 127);
   CHECK(rec._on[1].key == 65);
   CHECK(rec._on[1].velocity == 1);
   REQUIRE(rec._off.size() == 1);
   CHECK(rec._off[0].velocity == 0);
}

TEST_CASE("A MIDI 1.0 processor receives pressure and no other expression")
{
   recorder1 rec;
   rec.base().midi(
      packet_of(expression_event(CLAP_NOTE_EXPRESSION_PRESSURE, 0, 60, 1.0))
    , 0);
   rec.base().midi(
      packet_of(expression_event(CLAP_NOTE_EXPRESSION_TUNING, 0, 60, 0.5))
    , 0);
   rec.base().midi(
      packet_of(
         expression_event(CLAP_NOTE_EXPRESSION_BRIGHTNESS, 0, 60, 0.5))
    , 0);

   REQUIRE(rec._pressure.size() == 1);
   CHECK(rec._pressure[0].key == 60);
   CHECK(rec._pressure[0].velocity == 127);
   CHECK(rec._bends.empty());
   CHECK(rec._ccs.empty());
}

TEST_CASE("A MIDI 1.0 processor receives a MIDI 1.0 packet as it is")
{
   recorder1 rec;
   rec.base().midi(qplug::to_packet(midi2_event(0x20B00764u)), 0);

   REQUIRE(rec._ccs.size() == 1);
   CHECK(rec._ccs[0].key == 7);
   CHECK(rec._ccs[0].velocity == 100);
}

TEST_CASE("A MIDI 1.0 processor does not receive what MIDI 1.0 cannot say")
{
   recorder1 rec;
   auto const p = qplug::to_packet(midi2_event(0x40603C00u, 0x80000000u));
   rec.base().midi(p, 0);

   CHECK(rec._bends.empty());
   CHECK(rec._on.empty());
   CHECK(rec._ccs.empty());
}
