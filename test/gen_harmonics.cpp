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

constexpr q::frequency freq = 261.626; // 261.626;
constexpr float _1st_level = 0.3;      // Fundamental level
constexpr float _2nd_level = 0.4;      // Second harmonic level
constexpr float _3rd_level = 0.3;      // Third harmonic level

void write(std::string name, std::array<float, buffer_size>& buff)
{
   auto wav = audio_file::writer{
      "results/" + name + ".wav", audio_file::wav, audio_file::_16_bits
    , 1, sps // mono, 44100 sps
   };
   wav.write(buff);
}

void gen_harmonics_1(std::array<float, buffer_size>& buff)
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 1-second 261.626 Hz sine wave with harmonics

   auto sin1 = q::sin(freq, sps);
   auto sin2 = q::sin(freq * 2, sps);
   auto sin3 = q::sin(freq * 3, sps);
   for (auto& val : buff)
      val =
         _1st_level * sin1() +
         _2nd_level * sin2() +
         _3rd_level * sin3()
      ;

   write("harmonics_261", buff);
}

void gen_harmonics_2(std::array<float, buffer_size>& buff)
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 1-second 261.626 Hz sine wave with non-integer harmonics

   auto sin1 = q::sin(freq, sps);
   auto sin2 = q::sin(freq * 2.01, sps);
   auto sin3 = q::sin(freq * 3.02, sps);
   for (auto& val : buff)
      val =
         _1st_level * sin1() +
         _2nd_level * sin2() +
         _3rd_level * sin3()
      ;

   write("non_int_harmonics_261", buff);
}

int main()
{
   auto buff = std::array<float, buffer_size>{};

   gen_harmonics_1(buff);
   gen_harmonics_2(buff);

   return 0;
}
