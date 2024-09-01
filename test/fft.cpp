/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <q/fft/fft.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 48000;

TEST_CASE("Test_FFT")
{
   constexpr std::size_t p = 7;
   constexpr std::size_t n = 1<<p;
   constexpr std::size_t _2n = n*2;
   constexpr auto n_channels = 4;
   std::vector<float> out(_2n * n_channels);

   ////////////////////////////////////////////////////////////////////////////
   // sample data. A composite signal by summing three sine waves with
   // different frequencies and amplitudes.
   std::array<double, _2n> data;
   for (int i = 0; i < _2n; ++i)
   {
      data[i] =
         0.4 * std::sin(2_pi * i * 10 / _2n) +
         0.5 * std::sin(2_pi * i * 20 / _2n) +
         0.1 * std::sin(2_pi * i * 30 / _2n)
      ;

      auto pos = i * n_channels;
      out[pos] = data[i];
   }

   // Make a copy of the data
   auto orig = data;

   ////////////////////////////////////////////////////////////////////////////
   // compute FFT
   q::fft<n>(data.data());
   auto norm =
      [](double val)
      {
         return val / (n/2);
      };

   for (int i = 0; i < _2n; ++i)
   {
      auto pos = i * n_channels;
      auto ch2 = pos+1;
      out[ch2] = norm(data[i]);
   }

   REQUIRE_THAT(norm(data[0]),
      Catch::Matchers::WithinAbs(0, 1e-15));

   REQUIRE_THAT(norm(data[10*2]),
      Catch::Matchers::WithinRel(0.388, 0.01));

   REQUIRE_THAT(norm(data[20*2]),
      Catch::Matchers::WithinRel(0.44, 0.01));

   REQUIRE_THAT(norm(data[30*2]),
      Catch::Matchers::WithinRel(0.074, 0.01));

   ////////////////////////////////////////////////////////////////////////////
   // compute magnitude spectrum

   std::array<double, _2n> spectrum = orig;
   q::magspec<n>(spectrum.data());

   for (int i = 0; i < n/2+1; ++i)
   {
      auto pos = i * n_channels;
      auto ch3 = pos+2;
      out[ch3] = norm(spectrum[i]);
   }

   REQUIRE_THAT(norm(spectrum[10*2]),
      Catch::Matchers::WithinRel(0.388, 0.01));

   REQUIRE_THAT(norm(spectrum[20*2]),
      Catch::Matchers::WithinRel(0.44, 0.01));

   REQUIRE_THAT(norm(spectrum[30*2]),
      Catch::Matchers::WithinRel(0.074, 0.01));

   ////////////////////////////////////////////////////////////////////////////
   // compute Inverse FFT
   q::ifft<n>(data.data());

   for (int i = 0; i < _2n; ++i)
   {
      auto pos = i * n_channels;
      auto ch4 = pos+3;

      out[ch4] = data[i];
   }

   for (size_t i = 0; i < data.size(); ++i)
      Catch::Matchers::WithinAbs(
         std::abs(data[i] - orig[i]),
         std::numeric_limits<double>::epsilon()
      );

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/fft.wav", n_channels, sps
   );
   wav.write(out);
}
