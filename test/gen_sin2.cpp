/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <q/literals.hpp>
#include <q/synth.hpp>
#include <q/envelope.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
namespace audio_file = q::audio_file;
using namespace q::literals;

constexpr auto sps = 44100;
constexpr auto buffer_size = sps * 3;

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 3-second 440 Hz sine wave with ADSR envelope

   // Our envelope
   auto env =
      q::envelope(
        10_ms     // attack rate
      , 80_ms     // decay rate
      , -6_dB     // sustain level
      , 10_s      // sustain rate
      , 0.5_s     // release rate
      , sps
      );

   auto buff = std::array<float, buffer_size>{};   // The output buffer
   constexpr auto f = q::phase(440_Hz, sps);       // The synth frequency
   auto ph = q::phase();                           // Our phase accumulator

   env.trigger();                                  // Trigger note
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

   auto wav = audio_file::writer{
      "results/gen_sin2.wav", audio_file::wav, audio_file::_16_bits
    , 1, sps // mono, 44100 sps
   };
   wav.write(buff);

   return 0;
}
