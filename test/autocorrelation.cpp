/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
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

void process(std::string name, q::frequency lowest_freq, q::frequency highest_freq)
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
   max_val = std::abs(max_val);

   ////////////////////////////////////////////////////////////////////////////
   // Process
   constexpr auto n_channels = 3;
   std::vector<float> out(src.length() * n_channels);
   std::fill(out.begin(), out.end(), 0);
   auto i = out.begin();

   q::one_pole_lowpass        lp{ highest_freq, sps };
   q::peak                    pk{ 0.7, 0.0001 };
   q::peak_envelope_follower  env{ highest_freq.period() * 5, sps };
   q::bacf<>                  bacf{ lowest_freq, highest_freq, sps };
   std::size_t                ticks = 0;

   for (auto s : in)
   {
      // Normalize
      s *= 1.0 / max_val;

      // Low pass
      s = lp(s);
      *i++ = s;

      // Peaks
      auto p = pk(s, env(s));
      *i++ = p * 0.8;

      // Correlation
      bool proc = bacf(p);
      *i++ = -0.8;   // Default placeholder

      if (proc)
      {
         auto out_i = (i - (bacf.size() * n_channels)) - 1;
         auto const& info = bacf.result();
         for (auto n : info.correlation)
         {
            *out_i = n / float(info.max_count);
            out_i += n_channels;
         }
      }
      ++ticks;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/bacf_" + name + ".wav", audio_file::wav, audio_file::_16_bits
    , n_channels, sps
   };
   wav.write(out);
}

int main()
{
   process("sin_440", 200_Hz, 1500_Hz);
   process("1-Low E", 70_Hz, 400_Hz);
   process("2-Low E 2th", 70_Hz, 400_Hz);
   process("3-A", 100_Hz, 500_Hz);
   process("4-A 12th", 100_Hz, 500_Hz);
   process("5-D", 100_Hz, 600_Hz);
   process("6-D 12th", 100_Hz, 600_Hz);
   // process("7-G", 784.00_Hz);
   // process("8-G 12th", 784.00_Hz);
   // process("9-B", 987.76_Hz);
   // process("10-B 12th", 987.76_Hz);
   process("11-High E", 200_Hz, 1500_Hz);
   process("12-High E 12th", 200_Hz, 1500_Hz);

   process("Hammer-Pull High E", 200_Hz, 1500_Hz);
   process("Tapping D", 100_Hz, 600_Hz);
   process("Bend-Slide G", 180_Hz, 800_Hz);

   return 0;
}

