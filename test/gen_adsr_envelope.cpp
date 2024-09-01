/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/support/literals.hpp>
#include <q/synth/envelope_gen.hpp>
#include <q_io/audio_file.hpp>
#include <array>
#include "test.hpp"

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 48000;

TEST_CASE("TEST_adsr_envelope")
{
   ////////////////////////////////////////////////////////////////////////////
   // Generate an ADSR envelope using exponential ramps

   constexpr std::size_t size = sps * 4;
   constexpr auto n_channels = 2;
   constexpr auto buffer_size = size * n_channels;

   // Use vector to prevent stack overflow!
   static auto buff = std::vector<float>(buffer_size);   // The output buffer

   auto env_cfg = q::adsr_envelope_gen::config
   {
      300_ms,     // attack rate
      1_s,        // decay rate
      -12_dB,     // sustain level
      5_s,        // sustain rate
      1_s         // release rate
   };

   auto eg1 = q::adsr_envelope_gen{env_cfg, sps};
   auto eg2 = q::adsr_envelope_gen{env_cfg, sps};

   std::size_t sus1_end = q::as_float(2000_ms)*sps;
   std::size_t sus2_end = q::as_float(400_ms)*sps;

   eg2.decay_rate(400_ms, sps); // faster decay for eg2

   eg1.attack();
   eg2.attack();
   for (auto i = 0; i != size; ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;

      if (i == sus1_end)
         eg1.release();
      if (i == sus2_end)
         eg2.release();

      buff[ch1] = eg1();
      buff[ch2] = eg2();
   }

   {
      /////////////////////////////////////////////////////////////////////////
      // Write to a wav file

      q::wav_writer wav(
         "results/gen_adsr_envelope.wav", n_channels, sps // mono, 48000 sps
      );
      wav.write(buff);
   }
   compare_golden("gen_adsr_envelope", 1e-6);
}
