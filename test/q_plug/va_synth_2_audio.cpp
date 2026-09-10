/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// Stage 2 of the synth: the resonant filter the envelope sweeps. Stage 1's
// behaviour is covered by its own test; these are about the filter.
#define CATCH_CONFIG_MAIN
#include "plugin_harness.hpp"

namespace
{
   // The parameter ids stage 2 adds, as declared in its controller.
   constexpr clap_id attack_param = 1;
   constexpr clap_id decay_param = 2;
   constexpr clap_id sustain_param = 3;
   constexpr clap_id cutoff_param = 6;
   constexpr clap_id resonance_param = 7;
   constexpr clap_id depth_param = 8;
   constexpr clap_id f_attack_param = 9;
   constexpr clap_id f_decay_param = 10;
   constexpr clap_id f_sustain_param = 11;

   // How much of a note's energy is above a given harmonic, roughly: the
   // sum of the squares tells us how much sound there is, and a low-pass
   // that is doing its job leaves less of it.
   float energy(std::vector<float> const& v)
   {
      float sum = 0.0f;
      for (auto s : v)
         sum += s * s;
      return sum;
   }

   // Play one note with the filter set as given, and keep what came out.
   std::vector<float> play(
      instance& synth, double cutoff, double depth, double resonance
    , int blocks = 8)
   {
      param_events set_cutoff{cutoff_param, cutoff};
      synth.run(&set_cutoff._in);
      param_events set_depth{depth_param, depth};
      synth.run(&set_depth._in);
      param_events set_reso{resonance_param, resonance};
      synth.run(&set_reso._in);

      note_events on{60, 0, true};
      synth.run(&on._in);

      std::vector<float> out;
      for (int i = 0; i != blocks; ++i)
      {
         synth.run(nullptr);
         out.insert(out.end(), synth._left.begin(), synth._left.end());
      }
      return out;
   }
}

TEST_CASE("A closed filter passes less than an open one")
{
   // The same note, once with the filter wide open and once nearly shut,
   // and no envelope sweep either way so the cutoff is all that differs.
   instance open;
   auto const wide = play(open, 20000.0, 0.0, 0.7);

   instance closed;
   auto const shut = play(closed, 60.0, 0.0, 0.7);

   CHECK(energy(shut) < energy(wide) * 0.5f);
}

TEST_CASE("The envelope sweeps the cutoff, so depth is heard")
{
   // A closed filter with the envelope pushing it open passes more than a
   // closed filter that stays closed.
   instance swept;
   auto const with = play(swept, 100.0, 8.0, 0.7);

   instance still;
   auto const without = play(still, 100.0, 0.0, 0.7);

   CHECK(energy(with) > energy(without) * 2.0f);
}

TEST_CASE("Depth is counted in octaves above the cutoff")
{
   // Held at full level, an envelope of one means the cutoff sits exactly
   // one step of whatever unit depth is in. From 250 Hz, one octave is
   // 500 Hz and one decade would be 2500 Hz, so the swept note is asked
   // which of those two it sounds like.
   auto hold_open = [](instance& synth)
   {
      param_events sustain{sustain_param, 0.0};      // 0 dB: no decay
      synth.run(&sustain._in);
      param_events attack{attack_param, 0.001};
      synth.run(&attack._in);
      param_events decay{decay_param, 0.001};
      synth.run(&decay._in);
   };

   instance swept;
   hold_open(swept);
   auto const a = energy(play(swept, 250.0, 1.0, 0.7));

   instance low;
   hold_open(low);
   auto const b = energy(play(low, 500.0, 0.0, 0.7));

   instance high;
   hold_open(high);
   auto const c = energy(play(high, 2500.0, 0.0, 0.7));

   // Closer to the octave than to the decade, on a log scale so the
   // comparison is of ratios rather than of differences.
   auto const to_octave = std::abs(std::log(a / b));
   auto const to_decade = std::abs(std::log(a / c));
   CHECK(to_octave < to_decade);
}

TEST_CASE("Resonance lifts what sits at the cutoff")
{
   instance flat;
   auto const a = play(flat, 400.0, 0.0, 0.7);

   instance peaky;
   auto const b = play(peaky, 400.0, 0.0, 8.0);

   CHECK(energy(b) > energy(a));
}

TEST_CASE("Moving the cutoff while a note holds does not click")
{
   instance synth;

   // A long attack, so the note is still rising while the dial moves.
   param_events slow{attack_param, 2.0};
   synth.run(&slow._in);
   param_events decay{decay_param, 2.0};
   synth.run(&decay._in);

   auto tail = play(synth, 200.0, 0.0, 2.0, 2);

   // Now sweep the cutoff across the band, a step per block.
   for (double f : {400.0, 900.0, 2000.0, 4500.0, 9000.0, 18000.0})
   {
      param_events move{cutoff_param, f};
      synth.run(&move._in);
      tail.insert(tail.end(), synth._left.begin(), synth._left.end());
   }

   // A note at 60 is about 262 Hz, so a step from one sample to the next
   // is a small fraction of its swing. A click is a jump.
   CHECK(largest_step(tail) < 0.1f);
}

TEST_CASE("The filter leaves stage 1's envelope alone")
{
   // The controls stage 1 had keep their ids and their behaviour, so a
   // note still ends when it is released.
   instance synth;
   note_events on{60, 0, true};
   synth.run(&on._in);
   note_events off{60, 0, false};
   synth.run(&off._in);

   float peak = 1.0f;
   for (int i = 0; i != int(sps * 4 / block) && peak > 0.0f; ++i)
      peak = synth.run(nullptr);
   CHECK(peak == 0.0f);
}

////////////////////////////////////////////////////////////////////////////
// The filter's own contour: how bright the note is need not follow how
// loud it is.
////////////////////////////////////////////////////////////////////////////
TEST_CASE("The filter contour is not the amplifier's")
{
   // A note that is loud at once but opens slowly: with a fast amplifier
   // attack and a slow filter attack, the first blocks are loud and dark
   // and the later ones loud and bright.
   instance synth;

   param_events amp_fast{attack_param, 0.001};
   synth.run(&amp_fast._in);
   param_events amp_hold{sustain_param, 0.0};      // 0 dB: stays up
   synth.run(&amp_hold._in);
   param_events amp_decay{decay_param, 0.001};
   synth.run(&amp_decay._in);

   param_events f_slow{f_attack_param, 1.0};       // a second to open
   synth.run(&f_slow._in);
   param_events f_hold{f_sustain_param, 0.0};
   synth.run(&f_hold._in);

   auto const out = play(synth, 200.0, 6.0, 0.7, 40);

   // The opening blocks against the settled ones: the same loudness
   // either side, but the filter has swept open in between.
   auto const n = out.size() / 10;
   std::vector<float> early{out.begin(), out.begin() + n};
   std::vector<float> late{out.end() - n, out.end()};

   CHECK(energy(late) > energy(early) * 2.0f);
}

TEST_CASE("A filter contour that stays shut keeps the note dark")
{
   // The same note, the filter contour held down: loud, and dark
   // throughout, so the two halves are alike.
   instance synth;

   param_events amp_fast{attack_param, 0.001};
   synth.run(&amp_fast._in);
   param_events amp_hold{sustain_param, 0.0};
   synth.run(&amp_hold._in);
   param_events amp_decay{decay_param, 0.001};
   synth.run(&amp_decay._in);

   param_events f_slow{f_attack_param, 0.001};
   synth.run(&f_slow._in);
   param_events f_down{f_sustain_param, -60.0};    // shut at sustain
   synth.run(&f_down._in);
   param_events f_quick{f_decay_param, 0.005};
   synth.run(&f_quick._in);

   auto const out = play(synth, 200.0, 6.0, 0.7, 40);
   auto const n = out.size() / 10;
   std::vector<float> early{out.begin(), out.begin() + n};
   std::vector<float> late{out.end() - n, out.end()};

   CHECK(energy(late) < energy(early) * 2.0f);
}
