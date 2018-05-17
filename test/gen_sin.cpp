/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <q/literals.hpp>
#include <q/synth.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
namespace audio_file = q::audio_file;
using namespace q::literals;

auto constexpr sps = 44100;
auto constexpr buffer_size = sps;

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 1-second 440 Hz sine wave

   auto buff = std::array<float, buffer_size>{};   // The output buffer
   auto constexpr f = q::phase(440_Hz, sps);       // The synth frequency
   auto ph = q::phase();                           // Our phase accumulator

   for (auto& val : buff)
   {
      val = q::sin(ph);
      ph += f;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/gen_sin.wav", audio_file::wav, audio_file::_16_bits
    , 1, sps // mono, 44100 sps
   };
   wav.write(buff);

   return 0;
}
