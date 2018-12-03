/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/literals.hpp>
#include <q/synth.hpp>
#include <q_io/audio_stream.hpp>

namespace q = cycfi::q;
using namespace q::literals;

struct sin_synth : q::audio_stream
{
   sin_synth(q::frequency freq, std::size_t sps)
    : audio_stream(sps, 0, 2)
    , freq(freq, sps)
   {}

   virtual void process(in_channels const&, out_channels const& out) override
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame = 0; frame != out.frames(); ++frame, phase += freq)
      {
         right[frame] = left[frame] = q::sin(phase);
      }
   }

   q::phase phase;
   q::phase const freq;
};

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 5-second 440 Hz sine wave
   ////////////////////////////////////////////////////////////////////////////

   sin_synth synth{ 440_Hz, 44100 };

   synth.start();
   q::sleep(5_s);
   synth.stop();

   return 0;
}
