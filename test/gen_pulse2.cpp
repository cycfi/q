/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/literals.hpp>
#include <q/synth.hpp>
#include <q/envelope.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
namespace audio_file = q::audio_file;
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
         1_s       // attack rate
       , 2_s       // decay rate
       , -12_dB    // sustain level
       , 5_s       // sustain rate
       , 0.5_s     // release rate
      }
    , sps
   );

   auto buff = std::array<float, buffer_size>{};   // The output buffer
   constexpr auto f = q::phase(440_Hz, sps);       // The synth frequency
   auto ph = q::phase();                           // Our phase accumulator

   auto pulse = q::pulse;                          // Our pulse synth

   env.trigger(1.0f);                              // Trigger note
   for (auto i = 0; i != buffer_size; ++i)
   {
      auto& val = buff[i];
      if (i == buffer_size/2)                      // Release note
         env.release();
      pulse.width((env() * 0.6) + 0.3);            // Set pulse width
      val = pulse(ph, f) * env();
      ph += f;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::wav_writer{
      "results/gen_pulse2.wav", 1, sps // mono, 48000 sps
   };
   wav.write(buff);

   return 0;
}
