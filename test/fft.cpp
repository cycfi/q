/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/fft/fft.hpp>
#include <q_io/audio_file.hpp>
#include <array>

namespace q = cycfi::q;
using namespace q::literals;

constexpr auto sps = 48000;

int main()
{
   constexpr std::size_t p = 7;
   constexpr std::size_t n = 1<<p;
   constexpr std::size_t _2n = n * 2;
   constexpr auto n_channels = 3;
   std::vector<float> out(_2n * n_channels);

   ////////////////////////////////////////////////////////////////////////////
   // sample data
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

   ////////////////////////////////////////////////////////////////////////////
   // compute FFT
   q::fft<n>(data.data());

   for (int i = 0; i < _2n; ++i)
   {
      auto pos = i * n_channels;
      auto ch2 = pos+1;
      auto ch3 = pos+2;

      out[ch2] = data[i] / (n/2);
      out[ch3] = data[i+1] / (n/2);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/fft.wav", n_channels, sps
   );
   wav.write(out);

   return 0;
}
