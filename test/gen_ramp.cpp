/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/synth/blackman_gen.hpp>
#include <q/synth/hann_gen.hpp>
#include <q/synth/exponential_gen.hpp>
#include <q/synth/linear_gen.hpp>
#include <q/synth/ramp_gen.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 48000;

int main()
{
   ////////////////////////////////////////////////////////////////////////////
   // Generate an ADSR-like envelope using various tapers

   constexpr std::size_t size = sps;
   constexpr auto n_channels = 1;
   constexpr auto buffer_size = size * n_channels;

   constexpr auto sustain_level = 0.3f;
   constexpr auto release_duration = 400_ms;

   auto buff = std::array<float, buffer_size>{};   // The output buffer
   std::size_t sustain_end = size - (q::as_float(release_duration)*sps);

   auto attack = q::ramp_gen<q::blackman_upward_ramp_gen>{50_ms, sps};
   auto hold = q::ramp_gen<q::hold_line_gen>{25_ms, sps};
   auto decay = q::ramp_gen<q::hann_downward_ramp_gen>{200_ms, sps};
   auto sustain = q::ramp_gen<q::linear_decay_gen>{1000_ms, sps};
   auto release = q::ramp_gen<q::exponential_decay_gen>{release_duration, sps};

   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;

      if (!attack.done())
         buff[ch1] = attack(0.0f, 0.8f);
      else if (!hold.done())
         buff[ch1] = hold(0.8f, 0.0f);
      else if (!decay.done())
         buff[ch1] = decay(sustain_level, 0.8f-sustain_level);
      else if (i < sustain_end)
         buff[ch1] = sustain_level * sustain(0.0f, 1.0f);
      else
         buff[ch1] = release(0.0f, sustain_level) * sustain(0.0f, 1.0f);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/gen_adsr_ramps.wav", n_channels, sps // mono, 48000 sps
   );
   wav.write(buff);

   return 0;
}
