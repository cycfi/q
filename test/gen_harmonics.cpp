/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/literals.hpp>
#include <q/synth.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 44100;
constexpr auto buffer_size = sps;

constexpr float _1st_level = 0.3;      // Fundamental level
constexpr float _2nd_level = 0.4;      // Second harmonic level
constexpr float _3rd_level = 0.3;      // Third harmonic level

void write(std::string name, std::array<float, buffer_size>& buff)
{
   auto wav = q::wav_writer{
      "results/" + name + ".wav", 1, sps // mono, 44100 sps
   };
   wav.write(buff);
}

void gen_harmonics(
   char const* name, q::frequency freq, float h2, float h3
 , std::array<float, buffer_size>& buff
)
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 1-second sine wave with harmonics

   auto f1 = q::phase(freq, sps);
   auto f2 = q::phase(freq * h2, sps);
   auto f3 = q::phase(freq * h3, sps);
   auto ph1 = q::phase();
   auto ph2 = q::phase();
   auto ph3 = q::phase();

   for (auto& val : buff)
   {
      val =
         _1st_level * q::sin(ph1) +
         _2nd_level * q::sin(ph2) +
         _3rd_level * q::sin(ph3)
      ;
      ph1 += f1;
      ph2 += f2;
      ph3 += f3;
   }

   write(name, buff);
}

void gen_harmonics_1(std::array<float, buffer_size>& buff)
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 1-second 261.626 Hz sine wave with harmonics

   gen_harmonics("harmonics_261", 261.626_Hz, 2, 3, buff);
}

void gen_harmonics_2(std::array<float, buffer_size>& buff)
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 1-second 261.626 Hz sine wave with non-integer harmonics

   gen_harmonics("non_int_harmonics_261", 261.626_Hz, 2.003, 3, buff);
}

void gen_harmonics_3(std::array<float, buffer_size>& buff)
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 1-second 261.626 Hz sine wave with missing fundamental

   gen_harmonics("missing_fundamental_261", 261.626_Hz, 2, 3, buff);
}

void gen_harmonics_4(std::array<float, buffer_size>& buff)
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 1-second 1318.52 Hz sine wave with harmonics

   gen_harmonics("harmonics_1318", 1318.52_Hz, 2, 3, buff);
}

int main()
{
   auto buff = std::array<float, buffer_size>{};

   gen_harmonics_1(buff);
   gen_harmonics_2(buff);
   gen_harmonics_3(buff);
   gen_harmonics_4(buff);

   return 0;
}
