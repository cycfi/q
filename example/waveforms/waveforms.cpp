/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/saw_osc.hpp>
#include <q/synth/square_osc.hpp>
#include <q/synth/pulse_osc.hpp>
#include <q/synth/triangle_osc.hpp>
#include <q_io/audio_stream.hpp>
#include <q/utility/sleep.hpp>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
// Play Q's four bandwidth-limited oscillators in turn: sawtooth, square,
// pulse, and triangle, two seconds each, at a fixed pitch. A short fade
// brackets every segment so switching oscillators stays click-free.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;

struct waveform_synth : q::audio_stream
{
   waveform_synth(q::frequency freq, q::duration segment)
    : audio_stream(0, 2)
    , phase(freq, sampling_rate())
    , pulse(0.3f)                                             // 30% duty cycle
    , seg(std::size_t(as_double(segment) * sampling_rate()))
    , fade(std::size_t(0.02 * sampling_rate()))               // 20 ms
   {}

   void process(out_channels const& out)
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame : out.frames)
      {
         auto within = pos % seg;        // position within the current segment
         auto which = (pos / seg) % 4;   // 0:saw 1:square 2:pulse 3:triangle

         // Each oscillator takes the phase iterator and returns one
         // band-limited sample. q::saw, q::square and q::triangle are
         // stateless; pulse_osc carries its pulse width, so it is an object.
         float s = 0.0f;
         switch (which)
         {
            case 0: s = q::saw(phase);      break;
            case 1: s = q::square(phase);   break;
            case 2: s = pulse(phase);       break;
            case 3: s = q::triangle(phase); break;
         }
         ++phase;

         // Trapezoidal gain: fade in at the start of each segment and out at
         // the end, so the oscillator swap is never heard as a click.
         float g = 1.0f;
         if (within < fade)
            g = within / float(fade);
         else if (within >= seg - fade)
            g = (seg - within) / float(fade);

         left[frame] = right[frame] = s * g * 0.8f;
         ++pos;
      }
   }

   q::phase_iterator phase;     // The shared phase accumulator
   q::pulse_osc      pulse;     // Pulse is stateful (its width); the rest are not
   std::size_t       seg;       // Segment length, in samples
   std::size_t       fade;      // Fade length, in samples
   std::size_t       pos = 0;   // Running sample counter
};

int main()
{
   waveform_synth synth{220_Hz, 2_s};

   std::cout << "Bandwidth-limited oscillators, two seconds each:" << std::endl;
   synth.start();
   for (auto name : {"sawtooth", "square", "pulse (30%)", "triangle"})
   {
      std::cout << "  " << name << std::endl;
      q::sleep(2_s);
   }
   synth.stop();

   return 0;
}
