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
    , int blocks = 8, std::uint8_t key = 60)
   {
      param_events set_cutoff{cutoff_param, cutoff};
      synth.run(&set_cutoff._in);
      param_events set_depth{depth_param, depth};
      synth.run(&set_depth._in);
      param_events set_reso{resonance_param, resonance};
      synth.run(&set_reso._in);

      note_events on{key, 0, true};
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
      param_events sustain{sustain_param, 100.0};    // full: no decay
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
   param_events amp_hold{sustain_param, 100.0};    // stays up
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
   param_events amp_hold{sustain_param, 100.0};
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

////////////////////////////////////////////////////////////////////////////
// Pitch bend and the modulation wheel: what makes a keyboard feel joined
// to the sound. Bend is 14 bits centred on 8192, two semitones each way by
// the convention every keyboard ships with. The wheel adds vibrato.
////////////////////////////////////////////////////////////////////////////
namespace
{
   // A held note with the amplifier flat and the filter out of the way,
   // so pitch is all that varies. The estimator counts one upward zero
   // crossing per cycle, which a resonant filter parked near the note
   // would spoil with crossings of its own.
   void hold_flat(instance& synth)
   {
      param_events attack{1, 0.001};
      synth.run(&attack._in);
      param_events decay{2, 0.001};
      synth.run(&decay._in);
      param_events sustain{3, 100.0};
      synth.run(&sustain._in);
      param_events open{cutoff_param, 20000.0};
      synth.run(&open._in);
      param_events flat{resonance_param, 0.5};
      synth.run(&flat._in);
      param_events still{depth_param, 0.0};
      synth.run(&still._in);
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

////////////////////////////////////////////////////////////////////////////
// Velocity to the filter: a hard note opens the contour further.
////////////////////////////////////////////////////////////////////////////
namespace
{
   constexpr clap_id filter_velocity_param = 14;

   // Brightness of a settled note struck at the given velocity, with the
   // amplifier's own sensitivity off so loudness is not in the way.
   float bright_at(instance& synth, double velocity, double filter_sens)
   {
      param_events amp_off{velocity_param, 0.0};
      synth.run(&amp_off._in);
      param_events sens{filter_velocity_param, filter_sens};
      synth.run(&sens._in);
      param_events hold{sustain_param, 100.0};
      synth.run(&hold._in);
      param_events f_hold{f_sustain_param, 100.0};
      synth.run(&f_hold._in);
      param_events cut{cutoff_param, 200.0};
      synth.run(&cut._in);
      param_events dep{depth_param, 6.0};
      synth.run(&dep._in);
      param_events res{resonance_param, 0.7};
      synth.run(&res._in);

      note_events on{60, 0, true};
      on._ev.velocity = velocity;
      synth.run(&on._in);
      std::vector<float> out;
      for (int i = 0; i != 30; ++i)
      {
         synth.run(nullptr);
         if (i >= 20)
            out.insert(out.end(), synth._left.begin(), synth._left.end());
      }
      float e = 0, d = 0;
      for (std::size_t i = 1; i != out.size(); ++i)
      {
         e += out[i] * out[i];
         d += (out[i] - out[i-1]) * (out[i] - out[i-1]);
      }
      return d / std::max(1e-6f, e);
   }
}

TEST_CASE("With filter velocity a soft note is darker than a hard one")
{
   instance hard, soft;
   auto const a = bright_at(hard, 1.0, 100.0);
   auto const b = bright_at(soft, 0.2, 100.0);
   CHECK(b < a * 0.5f);
}

TEST_CASE("Without filter velocity a soft note is as bright as a hard one")
{
   instance hard, soft;
   auto const a = bright_at(hard, 1.0, 0.0);
   auto const b = bright_at(soft, 0.2, 0.0);
   CHECK(b == Approx(a).epsilon(0.05));
}

TEST_CASE("The cutoff follows the key at half an octave per octave")
{
   // Key tracking, always on. The cutoff sits below the note, so what
   // comes out is mostly the fundamental with the filter's slope on it.
   // Four poles is 24 dB per octave of distance above the cutoff, and
   // half tracking moves the cutoff half an octave for every octave of
   // pitch, so each octave up leaves the note half an octave further
   // out: 12 dB down in amplitude, a factor of sixteen in energy.
   //
   // Untracked, the cutoff would not move at all and each octave would
   // cost the full 24 dB, a factor of 256, which is what this separates.
   auto hold_open = [](instance& synth)
   {
      param_events fast{attack_param, 0.001};
      synth.run(&fast._in);
      param_events quick{decay_param, 0.001};
      synth.run(&quick._in);
      param_events full{sustain_param, 100.0};
      synth.run(&full._in);
   };

   auto at = [&](std::uint8_t key)
   {
      instance synth;
      hold_open(synth);
      return energy(play(synth, 100.0, 0.0, 0.7, 8, key));
   };

   auto const c4 = at(60);
   auto const c5 = at(72);
   auto const c6 = at(84);

   CHECK(c4 / c5 == Approx(16.0).epsilon(0.4));
   CHECK(c5 / c6 == Approx(16.0).epsilon(0.4));
}
