/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/sin.hpp>
#include <q/synth/envelope.hpp>
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

   env.trigger(1.0f);                              // Trigger note
   for (auto i = 0; i != buffer_size; ++i)
   {
      auto& val = buff[i];
      if (i == buffer_size/2)                      // Release note
         env.release();
      val = q::sin(ph) * env();
      ph += f;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/gen_sin2.wav", 1, sps // mono, 48000 sps
   );
   wav.write(buff);

   return 0;
}
