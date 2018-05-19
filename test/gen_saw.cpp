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

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Synthesize a 10-second band-limited square wave

   constexpr auto size = sps * 10;
   constexpr auto n_channels = 1;
   constexpr auto buffer_size = size * n_channels;

   auto buff = std::array<float, buffer_size>{};   // The output buffer
   constexpr auto f = q::phase(C[3], sps);         // The synth frequency
   auto ph = q::phase();                           // Our phase accumulator

   for (auto i = 0; i != size; ++i)
   {
      buff[i] = q::saw(ph) * 0.9;
      ph += f;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/gen_saw.wav", audio_file::wav, audio_file::_16_bits
    , n_channels, sps // mono, 44100 sps
   };
   wav.write(buff);

   return 0;
}
