/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// Stage 1 of the synth, driven through its CLAP entry.
#define CATCH_CONFIG_MAIN
#include "plugin_harness.hpp"

TEST_CASE("An instrument reports no audio input and one note port")
{
   instance synth;

   auto ports = static_cast<clap_plugin_audio_ports_t const*>(
      synth._plugin->get_extension(synth._plugin, CLAP_EXT_AUDIO_PORTS));
   REQUIRE(ports != nullptr);
   CHECK(ports->count(synth._plugin, true) == 0);      // no audio in
   CHECK(ports->count(synth._plugin, false) == 1);     // stereo out

   auto notes = static_cast<clap_plugin_note_ports_t const*>(
      synth._plugin->get_extension(synth._plugin, CLAP_EXT_NOTE_PORTS));
   REQUIRE(notes != nullptr);
   REQUIRE(notes->count(synth._plugin, true) == 1);

   clap_note_port_info_t info{};
   REQUIRE(notes->get(synth._plugin, 0, true, &info));
   CHECK((info.supported_dialects & CLAP_NOTE_DIALECT_MIDI) != 0);
   CHECK((info.supported_dialects & CLAP_NOTE_DIALECT_MIDI2) != 0);
   CHECK((info.supported_dialects & CLAP_NOTE_DIALECT_CLAP) != 0);
}

TEST_CASE("Silence in, silence out")
{
   instance synth;
   CHECK(synth.run(nullptr) == 0.0f);
}

TEST_CASE("A note makes a sound")
{
   instance synth;

   note_events on{60, 0, true};
   auto const peak = synth.run(&on._in);
   CHECK(peak > 0.01f);
}

TEST_CASE("A note keeps sounding after the block it started in")
{
   instance synth;

   note_events on{60, 0, true};
   synth.run(&on._in);
   CHECK(synth.run(nullptr) > 0.01f);
}

TEST_CASE("A note released falls silent")
{
   instance synth;

   note_events on{60, 0, true};
   synth.run(&on._in);

   note_events off{60, 0, false};
   synth.run(&off._in);

   // The release runs for the release time, so give it a second of blocks.
   float peak = 1.0f;
   for (int i = 0; i != int(sps / block) && peak > 0.0f; ++i)
      peak = synth.run(nullptr);
   CHECK(peak == 0.0f);
}

TEST_CASE("A note in the MIDI dialect makes a sound")
{
   instance synth;
   midi_events on{0x90, 60, 100};      // note on, channel 0, key 60
   CHECK(synth.run(&on._in) > 0.01f);
}

TEST_CASE("A note in the MIDI 2.0 dialect makes a sound")
{
   // The dialect this plugin says it prefers, so it is the one a host that
   // reads the note port will send.
   instance synth;
   midi2_events on{0x40903C00u, 0xFFFF0000u};
   CHECK(synth.run(&on._in) > 0.01f);
}

////////////////////////////////////////////////////////////////////////////
// The damper pedal, controller 64: while it is down, a note that is
// released keeps sounding, and lifting the pedal releases what it held.
////////////////////////////////////////////////////////////////////////////
TEST_CASE("The damper pedal holds a note through its note off")
{
   instance synth;

   midi_events pedal_down{0xB0, 64, 127};
   synth.run(&pedal_down._in);

   note_events on{60, 0, true};
   synth.run(&on._in);

   note_events off{60, 0, false};
   synth.run(&off._in);

   // A second of blocks: without the pedal the release would be long
   // gone, since the note off would have started it.
   float peak = 0.0f;
   for (int i = 0; i != int(sps / block); ++i)
      peak = synth.run(nullptr);
   CHECK(peak > 0.01f);
}

TEST_CASE("Lifting the damper pedal releases what it held")
{
   instance synth;

   midi_events pedal_down{0xB0, 64, 127};
   synth.run(&pedal_down._in);

   note_events on{60, 0, true};
   synth.run(&on._in);
   note_events off{60, 0, false};
   synth.run(&off._in);
   synth.run(nullptr);

   midi_events pedal_up{0xB0, 64, 0};
   synth.run(&pedal_up._in);

   float peak = 1.0f;
   for (int i = 0; i != int(sps * 4 / block) && peak > 0.0f; ++i)
      peak = synth.run(nullptr);
   CHECK(peak == 0.0f);
}

TEST_CASE("A pedal held note is not released by lifting the key twice")
{
   // The pedal is what decides, not how many note offs arrived.
   instance synth;

   midi_events pedal_down{0xB0, 64, 127};
   synth.run(&pedal_down._in);

   note_events on{60, 0, true};
   synth.run(&on._in);

   note_events off{60, 0, false};
   synth.run(&off._in);
   synth.run(&off._in);

   CHECK(synth.run(nullptr) > 0.01f);
}

TEST_CASE("A note struck again while the pedal is down sounds again")
{
   instance synth;

   midi_events pedal_down{0xB0, 64, 127};
   synth.run(&pedal_down._in);

   note_events on{60, 0, true};
   synth.run(&on._in);
   note_events off{60, 0, false};
   synth.run(&off._in);

   // Struck again: it must sound, and lifting the pedal must still end it.
   synth.run(&on._in);
   CHECK(synth.run(nullptr) > 0.01f);

   midi_events pedal_up{0xB0, 64, 0};
   synth.run(&pedal_up._in);
   note_events off2{60, 0, false};
   synth.run(&off2._in);

   float peak = 1.0f;
   for (int i = 0; i != int(sps * 4 / block) && peak > 0.0f; ++i)
      peak = synth.run(nullptr);
   CHECK(peak == 0.0f);
}

TEST_CASE("Half pedal counts as down, per the convention for controller 64")
{
   // 64 and above is down, below is up.
   instance synth;

   midi_events half{0xB0, 64, 64};
   synth.run(&half._in);

   note_events on{60, 0, true};
   synth.run(&on._in);
   note_events off{60, 0, false};
   synth.run(&off._in);

   float peak = 0.0f;
   for (int i = 0; i != int(sps / block); ++i)
      peak = synth.run(nullptr);
   CHECK(peak > 0.01f);
}

TEST_CASE("A voice stolen from the pedal is not released by lifting it")
{
   // Sixteen voices, all held by the pedal, then a seventeenth note: it
   // has to steal one of them. The stolen voice is now playing a key that
   // is still down, so lifting the pedal must not end it.
   instance synth;

   midi_events pedal_down{0xB0, 64, 127};
   synth.run(&pedal_down._in);

   for (int i = 0; i != 16; ++i)
   {
      note_events on{std::uint8_t(60 + i), 0, true};
      synth.run(&on._in);
   }
   for (int i = 0; i != 16; ++i)
   {
      note_events off{std::uint8_t(60 + i), 0, false};
      synth.run(&off._in);
   }

   note_events stealer{76, 0, true};      // the seventeenth, key still down
   synth.run(&stealer._in);

   midi_events pedal_up{0xB0, 64, 0};
   synth.run(&pedal_up._in);

   // Long enough for every released voice to have died away.
   float peak = 0.0f;
   for (int i = 0; i != int(sps * 2 / block); ++i)
      peak = synth.run(nullptr);

   CHECK(peak > 0.01f);
}

////////////////////////////////////////////////////////////////////////////
// The sustain level, moved from the panel. Both of these once failed in
// Q's envelope: the decay landed where the sustain used to be and the
// sustain could not hold below it, and a note already sitting in its
// sustain did not follow a level moved under it.
////////////////////////////////////////////////////////////////////////////
TEST_CASE("A sustain set low is heard low")
{
   // The envelope is built at -12 dB. Asking for -60 dB before the note
   // must be honoured, not floored at the level it was built with.
   instance synth;
   param_events quiet{3, -60.0};                // sustain, in dB
   synth.run(&quiet._in);
   param_events fast{1, 0.001};                 // attack
   synth.run(&fast._in);
   param_events decay{2, 0.01};
   synth.run(&decay._in);

   note_events on{60, 0, true};
   synth.run(&on._in);
   float peak = 0.0f;
   for (int i = 0; i != 40; ++i)
      peak = synth.run(nullptr);

   // -60 dB is a thousandth. -12 dB, the floor the bug left, is a quarter.
   CHECK(peak < 0.02f);
}

TEST_CASE("A sustain moved while a note is held follows the hand")
{
   instance synth;
   param_events fast{1, 0.001};
   synth.run(&fast._in);
   param_events decay{2, 0.01};
   synth.run(&decay._in);

   note_events on{60, 0, true};
   synth.run(&on._in);
   float before = 0.0f;
   for (int i = 0; i != 20; ++i)
      before = synth.run(nullptr);

   param_events quiet{3, -40.0};
   synth.run(&quiet._in);
   float after = 0.0f;
   for (int i = 0; i != 20; ++i)
      after = synth.run(nullptr);

   CHECK(after < before * 0.25f);
}
