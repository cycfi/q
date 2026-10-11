/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/onset_gate.hpp>
#include <q/support/literals.hpp>

namespace q = cycfi::q;
using namespace q::literals;

TEST_CASE("onset_gate: a falling envelope does not reopen the gate")
{
   // 500 us at 48 kHz: the slope looks 24 samples back. The level opens
   // the gate; the slope must still see the loud samples, so a quiet
   // envelope just after the release is not read as a rise from silence.
   auto g = q::onset_gate{-24_dB, -45_dB, -55_dB, 500_us, 48000};

   for (int i = 0; i != 100; ++i)
      CHECK(!g(0.0f));
   for (int i = 0; i != 1000; ++i)
      CHECK(g(0.5f));

   CHECK(!g(0.0015f));     // below the -55 dB release threshold
   CHECK(!g(0.007f));      // above -45 dB, but falling from 0.5
}

TEST_CASE("onset_gate: a fast rise below the onset threshold opens")
{
   auto g = q::onset_gate{-24_dB, -45_dB, -55_dB, 500_us, 48000};

   for (int i = 0; i != 100; ++i)
      CHECK(!g(0.0f));
   CHECK(g(0.007f));       // rise over -45 dB, level under -24 dB
}

TEST_CASE("onset_gate: a slow rise below the onset threshold stays shut")
{
   auto g = q::onset_gate{-24_dB, -45_dB, -55_dB, 500_us, 48000};

   // 0.0001 per sample: 0.0024 over 24 samples, under -45 dB (0.0056).
   float env = 0.0f;
   for (int i = 0; i != 500; ++i)
   {
      env += 0.0001f;
      CHECK(!g(env));
   }
}
