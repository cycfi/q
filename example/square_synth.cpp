/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/literals.hpp>
#include <q/synth.hpp>
#include <q/sfx.hpp>
#include <q/envelope.hpp>
#include <q_io/audio_stream.hpp>

///////////////////////////////////////////////////////////////////////////////
// Synthesize a 440 Hz bandwidth limited square wave with ADSR envelope that
// controls an amplifier and resonant filter.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;

struct square_synth : q::audio_stream
{
   square_synth(q::envelope::config env_cfg, q::frequency freq, std::size_t sps)
    : audio_stream(sps, 0, 2)
    , dt(freq, sps)
    , env(env_cfg, sps)
    , filter(0.5, 0.8)
    , filter_range(0.1, 0.99)
   {}

   virtual void process(out_channels const& out) override
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame = 0; frame != out.frames(); ++frame, phase += dt)
      {
         // Set the filter frequency
         auto cutoff = filter_range(env());
         filter.cutoff(cutoff);

         // Synthesize the square wave
         auto val = q::square(phase, dt);

         // Apply the envelope (amplifier and filter) with hard clip
         val = clip(filter(val) * env());

         // Output
         right[frame] = left[frame] = val;
      }
   }

   q::phase          phase;            // The phase accumulator
   q::phase const    dt;               // The phase delta
   q::envelope       env;              // The envelope
   q::reso_filter    filter;           // The resonant filter
   q::interpolate    filter_range;     // The resonant filter range
   q::clip           clip;             // Hard clip
};

int main()
{
   auto env_cfg = q::envelope::config
   {
      100_ms    // attack rate
    , 500_ms    // decay rate
    , -12_dB    // sustain level
    , 5_s       // sustain rate
    , 0.5_s     // release rate
   };

   square_synth synth{ env_cfg, 440_Hz, 44100 };

   synth.start();
   synth.env.trigger(1.0f);   // Trigger note
   q::sleep(5_s);
   synth.env.release();       // Release note
   q::sleep(2_s);
   synth.stop();

   return 0;
}
