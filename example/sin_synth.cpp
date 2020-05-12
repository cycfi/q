/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/sin.hpp>
#include <q_io/audio_stream.hpp>

///////////////////////////////////////////////////////////////////////////////
// Synthesize a 440 Hz sine wave.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;

struct sin_synth : q::port_audio_stream
{
   sin_synth(q::frequency freq)
    : port_audio_stream(0, 2)
    , phase(freq, this->sampling_rate())
   {}

   void process(out_channels const& out)
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame : out.frames())
      {
         // Synthesize the sin wave
         right[frame] = left[frame] = q::sin(phase++);
      }
   }

   q::phase_iterator phase;   // The phase iterator
};

int main()
{
   sin_synth synth{ 440_Hz };

   synth.start();
   q::sleep(5_s);
   synth.stop();

   return 0;
}
