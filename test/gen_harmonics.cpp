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
   // Synthesize a 1-second 261.626 Hz sine wave with harmonics

   constexpr q::frequency freq = 261.626; // 261.626;
   constexpr float _1st_level = 0.3;      // Fundamental level
   constexpr float _2nd_level = 0.4;      // Second harmonic level
   constexpr float _3rd_level = 0.3;      // Third harmonic level

   auto sin1 = q::sin(freq, sps);
   auto sin2 = q::sin(freq * 2, sps);
   auto sin3 = q::sin(freq * 3, sps);
   auto buff = std::array<float, buffer_size>{};
   for (auto& val : buff)
      val =
         _1st_level * sin1() +
         _2nd_level * sin2() +
         _3rd_level * sin3()
      ;

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/harmonics_261.wav", audio_file::wav, audio_file::_16_bits
    , 1, sps // mono, 44100 sps
   };
   wav.write(buff);

   return 0;
}
