/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q_io/audio_file.hpp>
#include <vector>

namespace q = cycfi::q;
namespace audio_file = q::audio_file;
using namespace q::literals;

void process(std::string name, q::frequency cutoff)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   auto src = audio_file::reader{"audio_files/" + name + ".aif"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   auto max_val = *std::max_element(in.begin(), in.end(),
      [](auto a, auto b) { return std::abs(a) < std::abs(b); }
   );

   ////////////////////////////////////////////////////////////////////////////
   // Detect waveform peaks

   constexpr auto n_channels = 3;
   std::vector<float> out(src.length() * n_channels);
   auto i = out.begin();

   q::one_pole_lowpass lp{ cutoff, sps };
   q::peak pk{ 0.7, 0.001 };
   q::peak_envelope_follower env{ cutoff.period() * 10, sps };

   for (auto s : in)
   {
      // Normalize
      s *= 1.0 / max_val;

      // Original
      //*i++ = s;

      // Low pass
      s = lp(s);
      *i++ = s;

      *i++ = pk(s, env(s)) * 0.8;
      *i++ = env();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/peak_" + name + ".wav", audio_file::wav, audio_file::_16_bits
    , n_channels, sps
   };
   wav.write(out);
}

int main()
{
   process("1-Low E", 329.64_Hz);
   process("2-Low E 2th", 329.64_Hz);
   process("3-A", 440.00_Hz);
   process("4-A 12th", 440.00_Hz);
   process("5-D", 587.32_Hz);
   process("6-D 12th", 587.32_Hz);
   process("7-G", 784.00_Hz);
   process("8-G 12th", 784.00_Hz);
   process("9-B", 987.76_Hz);
   process("10-B 12th", 987.76_Hz);
   process("11-High E", 1318.52_Hz);
   process("12-High E 12th", 1318.52_Hz);
   return 0;
}

