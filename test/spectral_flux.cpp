/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/fx/spectral_flux.hpp>
#include <q/support/literals.hpp>

#include <cmath>

namespace q = cycfi::q;
using namespace q::literals;

namespace
{
   constexpr auto sps = 48000.0f;

   // The level after half a second of a unit sine at frequency f
   float settled(q::spectral_flux& sf, double f)
   {
      float level = 0.0f;
      for (int i = 0; i != int(sps / 2); ++i)
         level = sf(float(std::sin(2 * q::pi * f * i / sps)));
      return level;
   }
}

TEST_CASE("spectral_flux: a tone below the cutoff barely registers")
{
   // 200 Hz against a 2 kHz, 12 dB/octave high-pass: about -40 dB.
   q::spectral_flux sf{2_kHz, 50_ms, sps};
   CHECK(settled(sf, 200.0) < 0.015f);
}

TEST_CASE("spectral_flux: a tone above the cutoff reads its amplitude")
{
   q::spectral_flux sf{2_kHz, 50_ms, sps};
   float level = settled(sf, 8000.0);
   CHECK(level > 0.95f);
   CHECK(level < 1.05f);
   CHECK(sf() == level);
}

TEST_CASE("spectral_flux: the level falls away when the band empties")
{
   q::spectral_flux sf{2_kHz, 50_ms, sps};
   float const top = settled(sf, 8000.0);

   // Silence: the peak follower releases over the window.
   float level = top;
   for (int i = 0; i != int(sps * 0.05f); ++i)
      level = sf(0.0f);
   CHECK(level < top * 0.3f);
   CHECK(level > 0.0f);
}
