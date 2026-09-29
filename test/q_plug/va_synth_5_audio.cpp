/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// Stage 5 of the synth. What it adds to stage 4 is presets, which are
// checked in their own test; the voice is stage 4's, so these are the
// same checks, kept so a change to the voice here is caught.
#define CATCH_CONFIG_MAIN
#include "plugin_harness.hpp"

namespace
{
   constexpr clap_id sustain_param = 3;
   constexpr clap_id chorus_mix_param = 17;
   constexpr clap_id volume_param = 18;

   // A held note at the given volume, in decibels. Returns the settled
   // peak, well after the slew has caught up.
   float peak_at(instance& synth, double db)
   {
      param_events attack{1, 0.001};
      synth.run(&attack._in);
      param_events decay{2, 0.001};
      synth.run(&decay._in);
      param_events hold{sustain_param, 100.0};
      synth.run(&hold._in);

      // Dry: the chorus sweeps the amplitude at its own rate, and a
      // block's peak would then depend on where in that sweep it fell.
      param_events dry{chorus_mix_param, 0.0};
      synth.run(&dry._in);
      param_events v{volume_param, db};
      synth.run(&v._in);

      note_events on{69, 0, true};
      synth.run(&on._in);
      float peak = 0.0f;
      for (int i = 0; i != 40; ++i)
         peak = synth.run(nullptr);
      return peak;
   }
}

TEST_CASE("The volume sets how loud the instrument is")
{
   instance loud, quiet;
   auto const a = peak_at(loud, 0.0);
   auto const b = peak_at(quiet, -12.0);

   // Twelve decibels down is a quarter of the amplitude.
   CHECK(b == Approx(a * 0.25f).epsilon(0.05));
}

TEST_CASE("The volume all the way down is silence")
{
   instance synth;
   CHECK(peak_at(synth, -60.0) < 0.005f);
}

TEST_CASE("The volume slews rather than steps")
{
   // Moved under a held note, it arrives smoothly: no sample-to-sample
   // jump big enough to hear as a click.
   instance synth;
   peak_at(synth, 0.0);

   param_events down{volume_param, -40.0};
   synth.run(&down._in);

   std::vector<float> tail;
   for (int i = 0; i != 20; ++i)
   {
      synth.run(nullptr);
      tail.insert(tail.end(), synth._left.begin(), synth._left.end());
   }
   CHECK(largest_step(tail) < 0.1f);
}

TEST_CASE("The volume arrives where it was sent")
{
   // Slewed, but not slewed forever: a second later it is where it was
   // asked to be.
   instance synth;
   peak_at(synth, 0.0);

   param_events down{volume_param, -12.0};
   synth.run(&down._in);
   float peak = 0.0f;
   for (int i = 0; i != int(sps / block); ++i)
      peak = synth.run(nullptr);

   instance settled;
   CHECK(peak == Approx(peak_at(settled, -12.0)).epsilon(0.05));
}
