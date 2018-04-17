/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q_io/audio_file.hpp>
#include <q/pitch_detector.hpp>
#include <q/pitch_detector.hpp>
#include <vector>
#include <iostream>

namespace q = cycfi::q;
namespace audio_file = q::audio_file;
using namespace q::literals;

constexpr auto pi = M_PI;
constexpr auto sps = 44100;               // 20000;

void process(
   std::vector<float> const& in
 , std::string name
 , q::frequency lowest_freq
 , q::frequency highest_freq)
{
   ////////////////////////////////////////////////////////////////////////////
   // Process
   constexpr auto n_channels = 2;
   std::vector<float> out(in.size() * n_channels);
   std::fill(out.begin(), out.end(), 0);

   q::pitch_detector<> pd{ lowest_freq, highest_freq, sps };
   auto const& bacf = pd.bacf();
   auto size = bacf.size();

   for (auto i = 0; i != in.size(); ++i)
   {
      auto s = in[i];
      out[i * n_channels] = s;
      out[(i * n_channels) + 1] = -1; // placeholder

      // Pitch Detection
      bool proc = pd(s);

      if (proc)
      {
         auto pos = i - size;
         auto oi = (out.begin() + pos * n_channels) + 1;

         auto const& info = bacf.result();
         for (auto n : info.correlation)
         {
            *oi = n / float(info.max_count);
            oi += n_channels;
         }

         auto frequency = pd.frequency();
         std::cout
            << frequency
            << " Error: "
            << 1200.0 * std::log2(frequency / 261.626) << " cents"
            << std::endl;
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/pitch_detect_" + name + ".wav", audio_file::wav, audio_file::_16_bits
    , n_channels, sps
   };
   wav.write(out);
}

int main()
{
   constexpr float freq = 261.626;           // 82.41;

   // These are in samples
   constexpr float period = float(sps) / freq;

   ////////////////////////////////////////////////////////////////////////////
   // Generate a test signal

   constexpr float _1st_level = 0.3;      // Fundamental level
   constexpr float _2nd_level = 0.4;      // Second harmonic level
   constexpr float _3rd_level = 0.3;      // Third harmonic level

   constexpr float offset = 0;
   std::size_t buff_size = 10000;

   std::vector<float> signal(buff_size);

   for (int i = 0; i < buff_size; i++)
   {
      auto angle = (i + offset) / period;
      signal[i] += _1st_level *  std::sin(2 * pi * angle);  // First harmonic
      signal[i] += _2nd_level *  std::sin(4 * pi * angle);  // Second harmonic
      signal[i] += _3rd_level *  std::sin(6 * pi * angle);  // Third harmonic
   }

   process(signal, "harmonics_261", 200_Hz, 1000_Hz);
   return 0;
}

