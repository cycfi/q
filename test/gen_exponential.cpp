/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/exponential_gen.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 48000;

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Generate a 1-cycle exponential curve

   constexpr std::size_t size = sps/100.0;
   constexpr auto n_channels = 2;
   constexpr auto buffer_size = size * n_channels;

   auto buff = std::array<float, buffer_size>{};   // The output buffer
   auto gen = q::exp_upward_ramp_gen{10_ms, sps};
   auto gen2 = q::exp_downward_ramp_gen{10_ms, sps};

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;

      buff[ch1] = gen();
      buff[ch2] = gen2();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/gen_exponential.wav", n_channels, sps // mono, 48000 sps
   );
   wav.write(buff);

   return 0;
}
