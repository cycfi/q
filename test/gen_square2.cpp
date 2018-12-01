/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/literals.hpp>
#include <q/synth.hpp>
#include <q/sfx.hpp>
#include <q/envelope.hpp>
#include <q/notes.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 48000;
constexpr auto buffer_size = sps * 10;

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 10-second pulse wave with ADSR envelope

   // Our envelope
   auto env = q::envelope(
      q::envelope::config
      {
         100_ms    // attack rate
       , 500_ms    // decay rate
       , -12_dB    // sustain level
       , 5_s       // sustain rate
       , 0.5_s     // release rate
      }
    , sps
   );

   auto buff = std::array<float, buffer_size>{};   // The output buffer
   constexpr auto f = q::phase(440_Hz, sps);       // The synth frequency
   auto ph = q::phase();                           // Our phase accumulator

   auto filt = q::reso_filter(0.5, 0.8);           // Our resonant filter(s)
   auto interp = q::interpolate(0.1, 0.99);        // Limits
   auto clip = q::clip();                          // Clipper

   env.trigger(1.0f);                              // Trigger note
   for (auto i = 0; i != buffer_size; ++i)
   {
      auto& val = buff[i];
      if (i == buffer_size/2)                      // Release note
         env.release();

      auto cutoff = interp(env());                 // Set the filter frequency
      filt.cutoff(cutoff);

      val = clip(filt(q::square(ph, f)) * env());
      ph += f;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = q::wav_writer{
      "results/gen_square2.wav", 1, sps // mono, 48000 sps
   };
   wav.write(buff);

   return 0;
}
