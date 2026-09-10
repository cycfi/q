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
   // The envelope is built at half level. Asking for one percent before
   // the note must be honoured, not floored at the level it was built
   // with.
   instance synth;
   param_events quiet{3, 1.0};                  // sustain, in percent
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

   // One percent; half, the floor the bug would leave, is fifty times it.
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

   param_events quiet{3, 5.0};
   synth.run(&quiet._in);
   float after = 0.0f;
   for (int i = 0; i != 20; ++i)
      after = synth.run(nullptr);

   CHECK(after < before * 0.25f);
}

////////////////////////////////////////////////////////////////////////////
// Pitch bend and the modulation wheel: what makes a keyboard feel joined
// to the sound. Bend is 14 bits centred on 8192, two semitones each way by
// the convention every keyboard ships with. The wheel adds vibrato.
////////////////////////////////////////////////////////////////////////////
namespace
{
   // A held note with the amplifier flat, so pitch is all that varies.
   void hold_flat(instance& synth)
   {
      param_events attack{1, 0.001};
      synth.run(&attack._in);
      param_events decay{2, 0.001};
      synth.run(&decay._in);
      param_events sustain{3, 100.0};
      synth.run(&sustain._in);
   }

   std::vector<float> collect(instance& synth, int blocks)
   {
      std::vector<float> out;
      for (int i = 0; i != blocks; ++i)
      {
         synth.run(nullptr);
         out.insert(out.end(), synth._left.begin(), synth._left.end());
      }
      return out;
   }

   // Bend as the three bytes a keyboard sends: status, lsb, msb.
   midi_events bend(std::uint16_t value)
   {
      return {0xE0, std::uint8_t(value & 0x7F), std::uint8_t(value >> 7)};
   }
}

TEST_CASE("Pitch bend at centre leaves the pitch alone")
{
   instance synth;
   hold_flat(synth);
   note_events on{69, 0, true};                 // A above middle C
   synth.run(&on._in);
   auto centre = bend(8192);
   synth.run(&centre._in);

   CHECK(frequency_of(collect(synth, 40), sps) == Approx(440.0).margin(4.0));
}

TEST_CASE("Pitch bend full up raises the pitch two semitones")
{
   instance synth;
   hold_flat(synth);
   note_events on{69, 0, true};
   synth.run(&on._in);
   auto up = bend(16383);
   synth.run(&up._in);

   // Two semitones above 440 is 493.9.
   CHECK(frequency_of(collect(synth, 40), sps)
      == Approx(493.9).margin(5.0));
}

TEST_CASE("Pitch bend full down lowers it two semitones")
{
   instance synth;
   hold_flat(synth);
   note_events on{69, 0, true};
   synth.run(&on._in);
   auto down = bend(0);
   synth.run(&down._in);

   CHECK(frequency_of(collect(synth, 40), sps)
      == Approx(392.0).margin(4.0));
}

TEST_CASE("A bend reaches a note that is already sounding")
{
   // Bend arrives after the note, not before it, and the note follows.
   instance synth;
   hold_flat(synth);
   note_events on{69, 0, true};
   synth.run(&on._in);
   collect(synth, 10);

   auto up = bend(16383);
   synth.run(&up._in);
   CHECK(frequency_of(collect(synth, 40), sps)
      == Approx(493.9).margin(5.0));
}

TEST_CASE("A new note takes the bend that is already in force")
{
   instance synth;
   hold_flat(synth);
   auto up = bend(16383);
   synth.run(&up._in);
   note_events on{69, 0, true};
   synth.run(&on._in);

   CHECK(frequency_of(collect(synth, 40), sps)
      == Approx(493.9).margin(5.0));
}

TEST_CASE("Without the wheel the pitch is steady")
{
   instance synth;
   hold_flat(synth);
   note_events on{69, 0, true};
   synth.run(&on._in);
   collect(synth, 5);

   CHECK(period_spread(collect(synth, 40)) < 1.02f);
}

TEST_CASE("The wheel up adds vibrato")
{
   instance synth;
   hold_flat(synth);
   note_events on{69, 0, true};
   synth.run(&on._in);
   midi_events wheel{0xB0, 1, 127};
   synth.run(&wheel._in);
   collect(synth, 5);

   // Half a semitone each way is about 6% between the extremes of the
   // period, well clear of a sample's worth of noise on a 100 sample
   // period.
   auto const spread = period_spread(collect(synth, 40));
   CHECK(spread > 1.03f);
   CHECK(spread < 1.12f);
}

TEST_CASE("The wheel back down takes the vibrato away")
{
   instance synth;
   hold_flat(synth);
   note_events on{69, 0, true};
   synth.run(&on._in);
   midi_events wheel_up{0xB0, 1, 127};
   synth.run(&wheel_up._in);
   collect(synth, 10);
   midi_events wheel_down{0xB0, 1, 0};
   synth.run(&wheel_down._in);
   collect(synth, 5);

   CHECK(period_spread(collect(synth, 40)) < 1.02f);
}

////////////////////////////////////////////////////////////////////////////
// Velocity sensitivity: how much of how hard the key was struck reaches
// the loudness. At full, a soft note is soft; at none, every note is the
// same; between, a mix of the two.
////////////////////////////////////////////////////////////////////////////
namespace
{
   constexpr clap_id velocity_param = 13;

   // A note struck with the given velocity, in the CLAP dialect's 0 to 1,
   // held with the amplifier flat. Returns the settled peak.
   float peak_at(instance& synth, double velocity)
   {
      hold_flat(synth);
      note_events on{69, 0, true};
      on._ev.velocity = velocity;
      synth.run(&on._in);
      float peak = 0.0f;
      for (int i = 0; i != 20; ++i)
         peak = synth.run(nullptr);
      return peak;
   }
}

TEST_CASE("At full sensitivity a soft note is soft")
{
   instance loud, soft;
   param_events full_a{velocity_param, 100.0};
   loud.run(&full_a._in);
   param_events full_b{velocity_param, 100.0};
   soft.run(&full_b._in);

   auto const a = peak_at(loud, 1.0);
   auto const b = peak_at(soft, 0.25);
   CHECK(b < a * 0.35f);
   CHECK(b > a * 0.15f);
}

TEST_CASE("At no sensitivity every note is the same")
{
   instance loud, soft;
   param_events none_a{velocity_param, 0.0};
   loud.run(&none_a._in);
   param_events none_b{velocity_param, 0.0};
   soft.run(&none_b._in);

   auto const a = peak_at(loud, 1.0);
   auto const b = peak_at(soft, 0.25);
   CHECK(b == Approx(a).epsilon(0.02));
}

TEST_CASE("Half sensitivity is half way between")
{
   // A quarter velocity at half sensitivity: half of full plus half of a
   // quarter, so five eighths of a hard note.
   instance loud, soft;
   param_events half_a{velocity_param, 50.0};
   loud.run(&half_a._in);
   param_events half_b{velocity_param, 50.0};
   soft.run(&half_b._in);

   auto const a = peak_at(loud, 1.0);
   auto const b = peak_at(soft, 0.25);
   CHECK(b == Approx(a * 0.625f).epsilon(0.05));
}

TEST_CASE("Sensitivity does not change a hard note")
{
   instance full, none;
   param_events f{velocity_param, 100.0};
   full.run(&f._in);
   param_events n{velocity_param, 0.0};
   none.run(&n._in);

   CHECK(peak_at(full, 1.0) == Approx(peak_at(none, 1.0)).epsilon(0.02));
}
