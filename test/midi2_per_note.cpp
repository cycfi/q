/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
// MIDI 2.0's per-note messages, read into the same note_pitch,
// note_pressure and note_timbre that MPE produces, so one synth hears both.
// M2-104-UM sections 4.2.3, 4.2.4, 4.2.12, 4.2.14 and Appendix A.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/per_note.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;
namespace midi2 = q::midi_2_0;

namespace
{
   struct recorder : midi2::processor
   {
      using midi2::processor::operator();

      struct expression
      {
         std::uint8_t   channel;
         std::uint8_t   key;
         float          value;
         std::uint32_t  id;
      };

      void operator()(midi::note_pitch m, std::size_t)
      { _pitch.push_back({m.channel(), m.key(), m.semitones(), m.id()}); }
      void operator()(midi::note_pressure m, std::size_t)
      { _pressure.push_back({m.channel(), m.key(), m.value(), m.id()}); }
      void operator()(midi::note_timbre m, std::size_t)
      { _timbre.push_back({m.channel(), m.key(), m.value(), m.id()}); }

      void operator()(midi2::note_on m, std::size_t)
      { _seen.push_back("note_on"); _keys.push_back(m.key()); }
      void operator()(midi2::note_off m, std::size_t)
      { _seen.push_back("note_off"); _keys.push_back(m.key()); }
      void operator()(midi2::per_note_pitch_bend, std::size_t)
      { _seen.push_back("per_note_pitch_bend"); }
      void operator()(midi2::poly_pressure, std::size_t)
      { _seen.push_back("poly_pressure"); }
      void operator()(midi2::registered_per_note_controller m, std::size_t)
      { _seen.push_back("rpnc"); _keys.push_back(m.index()); }
      void operator()(midi2::control_change m, std::size_t)
      { _seen.push_back("cc"); _keys.push_back(m.controller()); }
      void operator()(midi2::pitch_bend, std::size_t)
      { _seen.push_back("pitch_bend"); }
      void operator()(midi2::channel_pressure, std::size_t)
      { _seen.push_back("channel_pressure"); }

      float pitch_of(std::uint8_t key) const
      {
         for (auto i = _pitch.rbegin(); i != _pitch.rend(); ++i)
            if (i->key == key)
               return i->value;
         return -999.0f;
      }

      std::vector<expression>    _pitch;
      std::vector<expression>    _pressure;
      std::vector<expression>    _timbre;
      std::vector<std::string>   _seen;
      std::vector<std::uint8_t>  _keys;
   };

   constexpr std::uint32_t voice(
      std::uint8_t opcode, std::uint8_t channel
    , std::uint8_t byte3, std::uint8_t byte4)
   {
      return 0x40000000u | (std::uint32_t(opcode) << 20)
         | (std::uint32_t(channel) << 16) | (std::uint32_t(byte3) << 8) | byte4;
   }

   constexpr std::uint32_t centre = 0x80000000u;
   constexpr std::uint32_t half_up = 0xC0000000u;
   constexpr std::uint32_t full_down = 0x00000000u;

   struct fixture
   {
      void send(midi2::packet const& p)
      {
         midi2::dispatch(p, _time++, _chain);
      }

      void note_on(std::uint8_t ch, std::uint8_t key)
      {
         send({voice(0x9, ch, key, 0), 0xFFFF0000u});
      }

      void note_off(std::uint8_t ch, std::uint8_t key)
      {
         send({voice(0x8, ch, key, 0), 0u});
      }

      void note_bend(std::uint8_t ch, std::uint8_t key, std::uint32_t v)
      {
         send({voice(0x6, ch, key, 0), v});
      }

      void bend(std::uint8_t ch, std::uint32_t v)
      {
         send({voice(0xE, ch, 0, 0), v});
      }

      // Registered controller bank 0 index 0: pitch bend sensitivity, zero
      // extended, so the semitones sit in the top seven bits.
      void bend_range(std::uint8_t ch, std::uint8_t semitones)
      {
         send({voice(0x2, ch, 0, 0), std::uint32_t(semitones) << 25});
      }

      // Forget the initial state a note on reports; these tests are about
      // what follows it.
      void wipe()
      {
         _rec._pitch.clear();
         _rec._pressure.clear();
         _rec._timbre.clear();
      }

      recorder                            _rec;
      midi2::per_note_reader<recorder&>   _chain{_rec};
      std::size_t                         _time = 0;
   };
}

TEST_CASE("Notes pass through, and a note on reports its starting state")
{
   fixture f;
   f.note_on(0, 60);

   CHECK(f._rec._seen == std::vector<std::string>{"note_on"});
   REQUIRE(f._rec._pitch.size() == 1);
   CHECK(f._rec._pitch.front().value == Approx(0.0f));
   REQUIRE(f._rec._pressure.size() == 1);
   CHECK(f._rec._pressure.front().value == Approx(0.0f));
   REQUIRE(f._rec._timbre.size() == 1);
   CHECK(f._rec._timbre.front().value == Approx(0.5f).margin(0.01));
}

TEST_CASE("4.2.12 Per-note pitch bend belongs to its note")
{
   // Centred at 0x80000000; half way up with the default range of 2
   // semitones is one semitone.
   fixture f;
   f.note_on(0, 60);
   f.wipe();
   f.note_bend(0, 60, half_up);

   REQUIRE(f._rec._pitch.size() == 1);
   CHECK(f._rec._pitch.front().channel == 0);
   CHECK(f._rec._pitch.front().key == 60);
   CHECK(f._rec._pitch.front().value == Approx(1.0f).margin(0.001));

   // It arrived as note_pitch, not as the raw message.
   CHECK(f._rec._seen == std::vector<std::string>{"note_on"});
}

TEST_CASE("4.2.7 Registered controller 0 sets the bend range")
{
   fixture f;
   f.bend_range(0, 12);
   f.note_on(0, 60);
   f.wipe();
   f.note_bend(0, 60, half_up);

   CHECK(f._rec.pitch_of(60) == Approx(6.0f).margin(0.001));

   f.note_bend(0, 60, full_down);
   CHECK(f._rec.pitch_of(60) == Approx(-12.0f).margin(0.001));
}

TEST_CASE("4.2.14 Channel bend and per-note bend both modify the pitch")
{
   // "Messages that Modify Pitch Relatively from Any Existing Pitch State:
   // ... Per-Note Pitch Bend, Pitch Bend"
   fixture f;
   f.note_on(0, 60);
   f.note_on(0, 64);
   f.wipe();

   f.note_bend(0, 60, half_up);        // +1 on this note only
   f.bend(0, half_up);                 // +1 on the channel

   CHECK(f._rec.pitch_of(60) == Approx(2.0f).margin(0.001));
   CHECK(f._rec.pitch_of(64) == Approx(1.0f).margin(0.001));
   auto const& seen = f._rec._seen;
   CHECK(std::count(seen.begin(), seen.end(), "pitch_bend") == 0);
}

TEST_CASE("4.2.3 Poly pressure is the note's pressure")
{
   fixture f;
   f.note_on(0, 60);
   f.wipe();
   f.send({voice(0xA, 0, 60, 0), centre});

   REQUIRE(f._rec._pressure.size() == 1);
   CHECK(f._rec._pressure.front().key == 60);
   CHECK(f._rec._pressure.front().value == Approx(0.5f).margin(0.001));
}

TEST_CASE("4.2.10 Channel pressure adds to every sounding note")
{
   fixture f;
   f.note_on(0, 60);
   f.note_on(0, 64);
   f.wipe();
   f.send({voice(0xA, 0, 60, 0), centre});      // half on 60
   f.send({voice(0xD, 0, 0, 0), centre});       // half on the channel

   REQUIRE(f._rec._pressure.size() == 3);
   CHECK(f._rec._pressure[1].key == 60);
   CHECK(f._rec._pressure[1].value == Approx(1.0f).margin(0.001));
   CHECK(f._rec._pressure[2].key == 64);
   CHECK(f._rec._pressure[2].value == Approx(0.5f).margin(0.001));
}

TEST_CASE("Appendix A Registered per-note controller 74 is timbre")
{
   // Table 11: 74 is Sound Controller 5, Brightness, which is what MPE
   // carries on controller 74 too.
   fixture f;
   f.note_on(0, 60);
   f.wipe();
   f.send({voice(0x0, 0, 60, 74), 0xFFFFFFFFu});

   REQUIRE(f._rec._timbre.size() == 1);
   CHECK(f._rec._timbre.front().key == 60);
   CHECK(f._rec._timbre.front().value == Approx(1.0f).margin(0.001));
   CHECK(f._rec._seen == std::vector<std::string>{"note_on"});
}

TEST_CASE("Other per-note controllers pass through as they are")
{
   fixture f;
   f.note_on(0, 60);
   f.send({voice(0x0, 0, 60, 1), 0x80000000u});       // modulation

   CHECK(f._rec._seen == std::vector<std::string>{"note_on", "rpnc"});
   CHECK(f._rec._keys.back() == 1);
}

TEST_CASE("Expression before the note is kept for it")
{
   // 4.2.5: per-note controllers apply to future notes on that number.
   fixture f;
   f.note_bend(0, 60, half_up);
   CHECK(f._rec._pitch.empty());

   f.note_on(0, 60);
   CHECK(f._rec.pitch_of(60) == Approx(1.0f).margin(0.001));
}

TEST_CASE("A note stops following its number once it has ended")
{
   fixture f;
   f.note_on(0, 60);
   f.note_off(0, 60);
   f.wipe();
   f.note_bend(0, 60, half_up);

   CHECK(f._rec._pitch.empty());
   CHECK(f._rec._seen == std::vector<std::string>{"note_on", "note_off"});
}

TEST_CASE("Channels are independent")
{
   fixture f;
   f.note_on(0, 60);
   f.note_on(1, 60);
   f.wipe();
   f.bend(1, half_up);

   REQUIRE(f._rec._pitch.size() == 1);
   CHECK(f._rec._pitch.front().channel == 1);
}

TEST_CASE("A control change other than 74 passes through")
{
   fixture f;
   f.send({voice(0xB, 0, 7, 0), 0x80000000u});

   CHECK(f._rec._seen == std::vector<std::string>{"cc"});
   CHECK(f._rec._keys.back() == 7);
}

TEST_CASE("The note identifier is zero from this source")
{
   // MIDI 2.0 tells two notes of one number apart with per-note
   // management, not with an identifier. The field exists for sources
   // that have one.
   fixture f;
   f.note_on(0, 60);

   for (auto const& e : f._rec._pitch)
      CHECK(e.id == 0);
}
