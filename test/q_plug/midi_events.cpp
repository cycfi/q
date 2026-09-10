/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The host's note events, in each dialect CLAP offers, turned into the Q
// messages a processor answers. Cases are named for the clauses of
// clap/events.h and clap/ext/note-ports.h they come from.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <qplug/clap/midi_events.hpp>
#include <qplug/midi_processor.hpp>
#include <q/midi/packet_writer.hpp>

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

   // A processor that writes down what reached it, through the same base
   // an example would use.
   struct recorder : qplug::midi_processor<recorder>
   {
      using midi_processor::operator();

      struct note
      {
         std::uint8_t   channel;
         std::uint8_t   key;
         std::uint32_t  velocity;
         std::size_t    time;
      };

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

      void operator()(midi::sysex_view msg, std::size_t)
      {
         _sysex.assign(msg.data().begin(), msg.data().end());
      }

      std::vector<note>          _on;
      std::vector<note>          _off;
      std::vector<note>          _on2;
      std::vector<note>          _ccs;
      std::vector<std::uint8_t>  _sysex;

      // The framework hands MIDI to a processor through this base, never
      // to the derived type, so the tests do the same.
      qplug::processor& base() { return *this; }
   };
}

////////////////////////////////////////////////////////////////////////////
// The MIDI dialect: clap_event_midi, three bytes, already a MIDI message.
////////////////////////////////////////////////////////////////////////////
TEST_CASE("A MIDI dialect event is the message it already is")
{
   auto const raw = qplug::to_raw_message(midi_event(0x91, 60, 100));

   recorder rec;
   midi::dispatch(raw, 0, rec);

   REQUIRE(rec._on.size() == 1);
   CHECK(rec._on[0].channel == 1);
   CHECK(rec._on[0].key == 60);
   CHECK(rec._on[0].velocity == 100);
}

TEST_CASE("A MIDI dialect event of one or two bytes carries no more")
{
   // 0xD0, channel pressure, is two bytes. The third must not appear.
   auto const raw = qplug::to_raw_message(midi_event(0xD0, 64, 0x7F));
   CHECK((raw.data & 0xFF) == 0xD0);
   CHECK(((raw.data >> 8) & 0xFF) == 64);
   CHECK(((raw.data >> 16) & 0xFF) == 0);
}

////////////////////////////////////////////////////////////////////////////
// The CLAP dialect: clap_event_note, a typed note with a 0..1 velocity.
////////////////////////////////////////////////////////////////////////////
TEST_CASE("A CLAP note on becomes a MIDI note on")
{
   midi::raw_message raw;
   REQUIRE(qplug::to_raw_message(
      note_event(CLAP_EVENT_NOTE_ON, 2, 64, 1.0), raw));

   recorder rec;
   midi::dispatch(raw, 0, rec);

   REQUIRE(rec._on.size() == 1);
   CHECK(rec._on[0].channel == 2);
   CHECK(rec._on[0].key == 64);
   CHECK(rec._on[0].velocity == 127);
}

TEST_CASE("A CLAP note off becomes a MIDI note off")
{
   midi::raw_message raw;
   REQUIRE(qplug::to_raw_message(
      note_event(CLAP_EVENT_NOTE_OFF, 0, 60, 0.0), raw));

   recorder rec;
   midi::dispatch(raw, 0, rec);

   REQUIRE(rec._off.size() == 1);
   CHECK(rec._off[0].key == 60);
   CHECK(rec._off[0].velocity == 0);
}

TEST_CASE("Velocity scales across the range")
{
   struct { double in; std::uint8_t out; } const cases[] =
   {
      {1.0, 127}, {0.5, 64}, {0.0, 1}, {2.0, 127}, {-1.0, 1}
   };

   for (auto const& c : cases)
   {
      midi::raw_message raw;
      REQUIRE(qplug::to_raw_message(
         note_event(CLAP_EVENT_NOTE_ON, 0, 60, c.in), raw));
      CHECK(((raw.data >> 16) & 0xFF) == c.out);
   }
}

TEST_CASE("A CLAP note on of zero velocity is not a note off")
{
   // MIDI 1.0 reads a note on of velocity zero as a note off, so the
   // floor is one.
   midi::raw_message raw;
   REQUIRE(qplug::to_raw_message(
      note_event(CLAP_EVENT_NOTE_ON, 0, 60, 0.0), raw));

   recorder rec;
   midi::dispatch(raw, 0, rec);
   CHECK(rec._on.size() == 1);
   CHECK(rec._off.empty());
}

TEST_CASE("A wildcard addresses voices, not a MIDI message")
{
   // -1 in the tuple is a wildcard, which no three byte message can say.
   midi::raw_message raw;
   CHECK(!qplug::to_raw_message(
      note_event(CLAP_EVENT_NOTE_OFF, -1, 60, 0.0), raw));
   CHECK(!qplug::to_raw_message(
      note_event(CLAP_EVENT_NOTE_OFF, 0, -1, 0.0), raw));
}

TEST_CASE("A choke is a voice instruction, not a note")
{
   midi::raw_message raw;
   CHECK(!qplug::to_raw_message(
      note_event(CLAP_EVENT_NOTE_CHOKE, 0, 60, 0.0), raw));
}

////////////////////////////////////////////////////////////////////////////
// The MIDI 2.0 dialect: clap_event_midi2, four words, already a packet.
////////////////////////////////////////////////////////////////////////////
TEST_CASE("A MIDI 2.0 event is the packet it already is")
{
   clap_event_midi2_t ev{};
   ev.header.type = CLAP_EVENT_MIDI2;
   ev.data[0] = 0x40903C00u;
   ev.data[1] = 0xBEEF0000u;

   auto const p = qplug::to_packet(ev);
   CHECK(p.word(0) == 0x40903C00u);
   CHECK(p.words() == 2);

   recorder rec;
   rec.base().midi(p, 7);

   REQUIRE(rec._on2.size() == 1);
   CHECK(rec._on2[0].key == 60);
   CHECK(rec._on2[0].velocity == 0xBEEF);
   CHECK(rec._on2[0].time == 7);
}

TEST_CASE("A MIDI 1.0 packet reaches the MIDI 1.0 overloads")
{
   clap_event_midi2_t ev{};
   ev.header.type = CLAP_EVENT_MIDI2;
   ev.data[0] = 0x20B00764u;         // control change 7, value 100

   recorder rec;
   rec.base().midi(qplug::to_packet(ev), 0);

   REQUIRE(rec._ccs.size() == 1);
   CHECK(rec._ccs[0].key == 7);
   CHECK(rec._ccs[0].velocity == 100);
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
TEST_CASE("A processor hears only the overloads it writes")
{
   // Nothing here answers pitch bend; it must pass without a trace.
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
