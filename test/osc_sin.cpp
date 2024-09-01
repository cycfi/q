/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/sin_osc.hpp>
#include <q/synth/envelope_gen.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 48000;
constexpr auto buffer_size = sps * 10;

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 10-second sine wave with ADSR envelope

   // Our envelope
   auto env = q::adsr_envelope_gen(
      q::adsr_envelope_gen::config
      {
         1_s       // attack rate
       , 2_s       // decay rate
       , -12_dB    // sustain level
       , 10_s      // sustain rate
       , 1.5_s     // release rate
      }
    , sps
   );

   auto buff = std::array<float, buffer_size>{};   // The output buffer
   constexpr auto f = q::phase(440_Hz, sps);       // The synth frequency
   auto ph = q::phase();                           // Our phase accumulator

   env.attack();                                   // Start note
   for (auto i = 0; i != buffer_size; ++i)
   {
      auto& val = buff[i];
      if (i == buffer_size*0.8)                    // Release note
         env.release();
      val = q::sin(ph) * env();
      ph += f;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/synth_sin2.wav", 1, sps // mono, 48000 sps
   );
   wav.write(buff);

   return 0;
}
