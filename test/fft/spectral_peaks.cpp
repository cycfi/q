/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fft/spectral_peaks.hpp>
#include <q/fft/stft.hpp>

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace q = cycfi::q;

namespace
{
   constexpr std::size_t N = 64;
   using peaks_type = q::spectral_peaks;
   using mag_type = std::array<float, N/2+1>;

   // A spectrum of bumps: a triangle of `height` centred on each bin.
   mag_type bumps(std::vector<std::pair<std::size_t, float>> const& at)
   {
      mag_type m{};
      for (auto [k, height] : at)
      {
         for (std::size_t j = 0; j <= N/2; ++j)
         {
            float d = std::abs(float(j) - float(k));
            m[j] = std::max(m[j], height * std::max(0.0f, 1.0f - d / 4));
         }
      }
      return m;
   }

   // The regions cover 0 .. N/2 without a gap or an overlap.
   void require_tiling(peaks_type const& p)
   {
      auto peaks = p.peaks();
      REQUIRE(!peaks.empty());
      REQUIRE(peaks.front().first == 0);
      REQUIRE(peaks.back().last == N/2);
      for (std::size_t i = 0; i != peaks.size(); ++i)
      {
         REQUIRE(peaks[i].first <= peaks[i].bin);
         REQUIRE(peaks[i].bin <= peaks[i].last);
         if (i + 1 < peaks.size())
            REQUIRE(peaks[i+1].first == peaks[i].last + 1);
      }
   }
}

TEST_CASE("spectral_peaks finds each bump", "[spectral_peaks]")
{
   peaks_type p{N};
   auto m = bumps({{5, 1.0f}, {14, 0.5f}, {27, 0.8f}});
   p(m);

   auto peaks = p.peaks();
   REQUIRE(peaks.size() == 3);
   REQUIRE(peaks[0].bin == 5);
   REQUIRE(peaks[1].bin == 14);
   REQUIRE(peaks[2].bin == 27);
   require_tiling(p);
}

TEST_CASE("spectral_peaks: neighbours split at the valley",
   "[spectral_peaks]")
{
   // Bumps at 10 and 20 leave bins 14 to 16 at zero. The split is at the
   // first of the lowest bins, so the lower peak owns up to 14.
   peaks_type p{N};
   auto m = bumps({{10, 1.0f}, {20, 1.0f}});
   p(m);

   auto peaks = p.peaks();
   REQUIRE(peaks.size() == 2);
   REQUIRE(peaks[0].last == 14);
   REQUIRE(peaks[1].first == 15);
   require_tiling(p);
}

TEST_CASE("spectral_peaks: the threshold drops small peaks",
   "[spectral_peaks]")
{
   peaks_type p{N, 0.6f};
   auto m = bumps({{5, 1.0f}, {14, 0.5f}, {27, 0.8f}});
   p(m);

   auto peaks = p.peaks();
   REQUIRE(peaks.size() == 2);
   REQUIRE(peaks[0].bin == 5);
   REQUIRE(peaks[1].bin == 27);
   require_tiling(p);
}

TEST_CASE("spectral_peaks: no peaks in a flat spectrum",
   "[spectral_peaks]")
{
   peaks_type p{N};
   mag_type m;
   m.fill(1.0f);
   p(m);
   REQUIRE(p.peaks().empty());
}

TEST_CASE("spectral_peaks: one peak owns everything",
   "[spectral_peaks]")
{
   peaks_type p{N};
   auto m = bumps({{0, 1.0f}});
   p(m);
   REQUIRE(p.peaks().size() == 1);
   REQUIRE(p.peaks()[0].bin == 0);
   require_tiling(p);
}

TEST_CASE("spectral_peaks: the partials of a chord",
   "[spectral_peaks]")
{
   // Three tones through an stft: each shows up as a peak at its bin.
   constexpr std::size_t M = 1024;
   q::stft s{q::stft_window::hann(M, 256), 256};
   q::spectral_peaks p{M, 1.0f};
   std::array<float, M/2+1> mag;

   auto analyze = [&](q::stft::bins_type bins)
   {
      for (std::size_t k = 0; k <= M/2; ++k)
         mag[k] = std::hypot(bins[2*k], bins[2*k + 1]);
      p(mag);
   };

   for (std::size_t t = 0; t != 4*M; ++t)
   {
      double a = 2 * std::numbers::pi * t / M;
      s(float(std::sin(a * 40) + std::sin(a * 50) + std::sin(a * 60)),
         analyze);
   }

   auto peaks = p.peaks();
   REQUIRE(peaks.size() == 3);
   REQUIRE(peaks[0].bin == 40);
   REQUIRE(peaks[1].bin == 50);
   REQUIRE(peaks[2].bin == 60);
}
