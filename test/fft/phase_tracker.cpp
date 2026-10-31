/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fft/phase_tracker.hpp>
#include <q/fft/stft.hpp>
#include <q/support/base.hpp>

#include <array>
#include <vector>
#include <cmath>
#include <numbers>

namespace q = cycfi::q;
using Catch::Matchers::WithinAbs;

namespace
{
   // Runs a sine of `freq` bins through an stft and a phase_tracker, and
   // returns the tracked frequency of the strongest bin, last frame.
   float track(
      std::size_t N, double freq, std::size_t hop, std::size_t frames)
   {
      q::stft s{q::stft_window::hann(N, hop), hop};
      q::phase_tracker tracker{N, hop};
      std::vector<float> phase(N/2+1), inst(N/2+1);
      float result = 0;

      auto analyze = [&](q::stft::bins_type bins)
      {
         std::size_t peak = 0;
         float best = 0;
         for (std::size_t k = 0; k <= N/2; ++k)
         {
            phase[k] = q::fast_atan2(bins[2*k + 1], bins[2*k]);
            auto m = std::hypot(bins[2*k], bins[2*k + 1]);
            if (m > best)
            {
               best = m;
               peak = k;
            }
         }
         tracker(phase, inst);
         result = inst[peak];
      };

      for (std::size_t t = 0; t != frames * hop; ++t)
         s(float(std::sin(2 * std::numbers::pi * freq * t / N)), analyze);
      return result;
   }
}

TEST_CASE("phase_tracker finds a sine between bins", "[phase_tracker]")
{
   REQUIRE_THAT(track(1024, 20.3, 256, 16), WithinAbs(20.3, 0.01));
   REQUIRE_THAT(track(1024, 20.7, 256, 16), WithinAbs(20.7, 0.01));
   REQUIRE_THAT(track(1024, 100.5, 128, 16), WithinAbs(100.5, 0.01));
   REQUIRE_THAT(track(2048, 7.25, 512, 16), WithinAbs(7.25, 0.01));
}

TEST_CASE("phase_tracker: a sine on a bin centre", "[phase_tracker]")
{
   REQUIRE_THAT(track(512, 32.0, 128, 16), WithinAbs(32.0, 1e-3));
}

TEST_CASE("phase_tracker: the first frame gives bin centres",
   "[phase_tracker]")
{
   constexpr std::size_t N = 64;
   q::phase_tracker tracker{N, 16};
   std::array<float, N/2+1> phase, inst;
   for (std::size_t k = 0; k <= N/2; ++k)
      phase[k] = 1.0f + 0.1f * k;

   tracker(phase, inst);
   for (std::size_t k = 0; k <= N/2; ++k)
      REQUIRE(inst[k] == float(k));

   tracker.reset();
   tracker(phase, inst);
   for (std::size_t k = 0; k <= N/2; ++k)
      REQUIRE(inst[k] == float(k));
}

TEST_CASE("phase_tracker: a steady extra advance is a fixed offset",
   "[phase_tracker]")
{
   // Each frame, every bin's phase runs an eighth of a turn ahead of its
   // centre advance: an offset of (pi/4) N / (2 pi hop) = 0.5 bin at a hop
   // of N/4.
   constexpr std::size_t N = 256;
   constexpr std::size_t hop = N/4;
   q::phase_tracker tracker{N, hop};
   std::array<float, N/2+1> phase, inst;

   for (int frame = 0; frame != 4; ++frame)
   {
      for (std::size_t k = 0; k <= N/2; ++k)
      {
         double centre = 2 * q::pi * double(k) * hop / N;
         phase[k] = q::principal_arg(float(frame * (centre + q::pi/4)));
      }
      tracker(phase, inst);
   }
   for (std::size_t k = 0; k <= N/2; ++k)
      REQUIRE_THAT(inst[k], WithinAbs(k + 0.5, 1e-3));
}
