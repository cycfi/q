/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The FM synth played through its CLAP entry: a note at its pitch, the
// parameters reaching the patch the voices play, and what the plugin adds
// around Q's voice: the operator switches, bend, transpose, mono.
#define CATCH_CONFIG_MAIN
#include "plugin_harness.hpp"

#include <cmath>
#include <vector>

namespace
{
   constexpr clap_id transpose_param = 19;
   constexpr clap_id volume_param = 20;
   constexpr clap_id keys_param = 21;
   constexpr clap_id op1_level = 100;
   constexpr clap_id op1_on = 121;

   void set(instance& synth, clap_id id, double value)
   {
      param_events e{id, value};
      synth.run(&e._in);
   }

   // A note held for a while, then a run of what it sounds like
   std::vector<float> play(
      instance& synth, std::uint8_t key, int settle = 8, int blocks = 8)
   {
      note_events on{key, 0, true};
      synth.run(&on._in);
      for (int i = 0; i != settle; ++i)
         synth.run(nullptr);

      std::vector<float> out;
      for (int i = 0; i != blocks; ++i)
      {
         synth.run(nullptr);
         out.insert(out.end(), synth._left.begin(), synth._left.end());
      }
      return out;
   }

   float peak_of(std::vector<float> const& v)
   {
      float peak = 0.0f;
      for (auto s : v)
         peak = std::max(peak, std::abs(s));
      return peak;
   }

   float cents(float f, float ref)
   {
      return 1200.0f * std::log2(f / ref);
   }
}

TEST_CASE("The initial voice plays the key's pitch")
{
   // OP1 alone, a sine: the key's frequency and nothing else
   instance synth;
   auto const y = play(synth, 69);
   CHECK(peak_of(y) > 0.01f);
   CHECK(std::abs(cents(frequency_of(y, sps), 440.0f)) < 5.0f);
}

TEST_CASE("An operator at level 0 is silent")
{
   instance synth;
   set(synth, op1_level, 0.0);
   CHECK(peak_of(play(synth, 69)) < 1e-4f);
}

TEST_CASE("A switched-off operator is silent")
{
   instance synth;
   set(synth, op1_on, 0.0);
   CHECK(peak_of(play(synth, 69)) < 1e-4f);
}

TEST_CASE("Transpose moves the pitch by semitones")
{
   instance synth;
   set(synth, transpose_param, 12.0);
   auto const y = play(synth, 69);
   CHECK(std::abs(cents(frequency_of(y, sps), 880.0f)) < 5.0f);
}

TEST_CASE("The bend reaches two semitones")
{
   instance synth;
   midi_events bend{0xE0, 0x7F, 0x7F};
   synth.run(&bend._in);
   auto const y = play(synth, 69);
   auto const up = 440.0f * std::exp2(2.0f / 12.0f);
   CHECK(std::abs(cents(frequency_of(y, sps), up)) < 5.0f);
}

TEST_CASE("Mono plays one note, the last key down")
{
   instance synth;
   set(synth, keys_param, 1.0);
   note_events first{57, 0, true};
   synth.run(&first._in);
   auto const y = play(synth, 69);
   CHECK(std::abs(cents(frequency_of(y, sps), 440.0f)) < 5.0f);
}

TEST_CASE("The volume sets how loud it is")
{
   instance loud;
   auto const full = peak_of(play(loud, 69, 60));

   instance soft;
   set(soft, volume_param, -20.0);
   auto const down = peak_of(play(soft, 69, 60));

   REQUIRE(full > 0.0f);
   CHECK(20.0f * std::log10(down / full) == Approx(-20.0f).margin(0.5f));
}

TEST_CASE("The mod wheel brings in the vibrato")
{
   // No LFO depth of the patch's own, the pitch fully sensitive: only
   // the wheel moves the pitch. A second of it, a few LFO cycles.
   constexpr clap_id pitch_mod_sens = 17;
   auto spread = [](bool wheel)
   {
      instance synth;
      set(synth, pitch_mod_sens, 7.0);
      if (wheel)
      {
         midi_events cc{0xB0, 1, 127};
         synth.run(&cc._in);
      }
      return period_spread(play(synth, 69, 8, 90));
   };

   // Steady, the periods are 100 and 101 samples (440 Hz at 44.1 kHz
   // is 100.2), so a spread of 1.01 is no vibrato at all.
   CHECK(spread(false) < 1.02f);
   CHECK(spread(true) > 1.1f);
}

TEST_CASE("Switching an operator off silences the note it is in")
{
   instance synth;
   note_events on{69, 0, true};
   synth.run(&on._in);
   float before = 0.0f;
   for (int i = 0; i != 8; ++i)
      before = std::max(before, synth.run(nullptr));
   REQUIRE(before > 0.01f);

   // The note is held; OP1, its only carrier, goes off. What is left
   // is the output stage's high-pass settling, gone in a few blocks.
   set(synth, op1_on, 0.0);
   for (int i = 0; i != 8; ++i)
      synth.run(nullptr);
   float after = 0.0f;
   for (int i = 0; i != 8; ++i)
      after = std::max(after, synth.run(nullptr));
   CHECK(after < 1e-4f);

   // And back on, the held note sounds again
   set(synth, op1_on, 1.0);
   float again = 0.0f;
   for (int i = 0; i != 8; ++i)
      again = std::max(again, synth.run(nullptr));
   CHECK(again > 0.01f);
}

TEST_CASE("A sounding note takes a patch change where it is")
{
   // OP1's release level at 99 holds the note after its key is up, as
   // the DX7's TRAIN does. Lowering it, as a new patch would, lets the
   // held note fade, as on a DX7; it is not cut, and not left droning.
   constexpr clap_id op1_l4 = 112;
   instance synth;
   set(synth, op1_l4, 99.0);

   note_events on{69, 0, true};
   synth.run(&on._in);
   for (int i = 0; i != 8; ++i)
      synth.run(nullptr);
   note_events off{69, 0, false};
   synth.run(&off._in);
   float held = 0.0f;
   for (int i = 0; i != 16; ++i)
      held = std::max(held, synth.run(nullptr));
   REQUIRE(held > 0.01f);

   set(synth, op1_l4, 0.0);
   for (int i = 0; i != 16; ++i)
      synth.run(nullptr);
   float after = 0.0f;
   for (int i = 0; i != 8; ++i)
      after = std::max(after, synth.run(nullptr));
   CHECK(after < 1e-4f);
}
