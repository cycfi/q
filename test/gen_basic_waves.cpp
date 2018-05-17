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

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 1-second 440 Hz pulse and saw waves

   auto constexpr size = sps;
   auto constexpr n_channels = 3;
   auto constexpr buffer_size = size * n_channels;

   auto buff = std::array<float, buffer_size>{};   // The output buffer
   auto constexpr f = q::phase(440_Hz, sps);       // The synth frequency
   auto ph = q::phase();                           // Our phase accumulator

   auto constexpr pulse = q::pulse_synth(0.4);     // Our pulse synth
   auto constexpr saw = q::saw;                    // Our saw synth
   auto constexpr tri = q::tri;                    // Our tri synth

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;

      buff[ch1] = pulse(ph);
      buff[ch2] = saw(ph);
      buff[ch3] = tri(ph);

      ph += f;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/gen_basic_waves.wav", audio_file::wav, audio_file::_16_bits
    , n_channels, sps // mono, 44100 sps
   };
   wav.write(buff);

   return 0;
}
