/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <q/literals.hpp>
#include <q/synth.hpp>
#include <q_io/audio_file.hpp>
#include <q/notes.hpp>
#include <array>

namespace q = cycfi::q;
namespace audio_file = q::audio_file;
using namespace q::literals;
using namespace q::notes;

constexpr auto sps = 48000;
constexpr auto buffer_size = sps * 10;

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 10-second sine wave

   auto buff = std::array<float, buffer_size>{};   // The output buffer
   constexpr auto f = q::phase(C[3], sps);         // The synth frequency
   auto ph = q::phase();                           // Our phase accumulator

   for (auto& val : buff)
   {
      val = q::sin(ph);
      ph += f;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/gen_sin.wav", 1, sps // mono, 48000 sps
   };
   wav.write(buff);

   return 0;
}
