/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/noise_gen.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 48000;

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Generate a 10-second noise

   constexpr std::size_t size = sps * 10;
   constexpr auto n_channels = 2;
   constexpr auto buffer_size = size * n_channels;

   auto buff = std::array<float, buffer_size>{};   // The output buffer
   auto pink_noise = q::pink_noise_gen{};
   auto white_noise = q::white_noise_gen{};

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;

      buff[ch1] = white_noise() * 0.5;
      buff[ch2] = pink_noise() * 0.5;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/gen_noise.wav", n_channels, sps // mono, 48000 sps
   );
   wav.write(buff);

   return 0;
}
