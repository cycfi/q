/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/blackman_gen.hpp>
#include <q/synth/hann_gen.hpp>
#include <q/synth/exponential_gen.hpp>
#include <q/synth/linear_gen.hpp>
#include <q/synth/envelope_gen.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 48000;

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Generate an ADSR-like envelope using various tapers

   constexpr std::size_t size = sps * 4;
   constexpr auto n_channels = 1;
   constexpr auto buffer_size = size * n_channels;

   auto buff = std::array<float, buffer_size>{};   // The output buffer

   auto env_cfg = q::adsr_envelope_gen::config
   {
      300_ms      // attack rate
    , 1_s         // decay rate
    , -12_dB      // sustain level
    , 5_s         // sustain rate
    , 1_s         // release rate
   };

   auto env_gen = q::adsr_envelope_gen{env_cfg, sps};

   std::size_t sustain_end = q::as_float(2000_ms)*sps;

   env_gen.attack();
   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;

      if (i == sustain_end)
         env_gen.release();

      buff[ch1] = env_gen();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/gen_adsr_envelope.wav", n_channels, sps // mono, 48000 sps
   );
   wav.write(buff);

   return 0;
}
