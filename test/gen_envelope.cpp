/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/support/literals.hpp>
#include <q/synth/blackman_gen.hpp>
#include <q/synth/hann_gen.hpp>
#include <q/synth/exponential_gen.hpp>
#include <q/synth/linear_gen.hpp>
#include <q/synth/envelope_gen.hpp>
#include <q_io/audio_file.hpp>
#include <array>
#include "test.hpp"

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 48000;

TEST_CASE("TEST_envelope")
{
   ////////////////////////////////////////////////////////////////////////////
   // Generate an ADSR-like envelope using various tapers

   constexpr std::size_t size = sps;
   constexpr auto n_channels = 1;
   constexpr auto buffer_size = size * n_channels;

   constexpr auto sustain_level = 0.3f;
   constexpr auto release_duration = 400_ms;

   auto buff = std::array<float, buffer_size>{};   // The output buffer

   auto env_gen =
      q::envelope_gen{
         q::make_envelope_segment<q::blackman_upward_ramp_gen>(50_ms, 0.8f, sps)          // Attack
       , q::make_envelope_segment<q::hold_line_gen>(25_ms, 0.8f, sps)                     // Hold
       , q::make_envelope_segment<q::hann_downward_ramp_gen>(200_ms, 0.3f, sps)           // Decay
       , q::make_envelope_segment<q::lin_downward_ramp_gen>(1000_ms, 0.0f, sps)           // Sustain
       , q::make_envelope_segment<q::exp_downward_ramp_gen>(release_duration, 0.0f, sps)  // Release
      };

   env_gen.reset();

   std::size_t sustain_end = size - (q::as_float(release_duration)*sps);

   env_gen.attack();
   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;

      if (i == sustain_end)
         env_gen.release();

      buff[ch1] = env_gen();
   }

   {
      ////////////////////////////////////////////////////////////////////////
      // Write to a wav file

      q::wav_writer wav(
         "results/gen_envelope.wav", n_channels, sps // mono, 48000 sps
      );
      wav.write(buff);
   }
   compare_golden("gen_envelope", 1e-6);
}
