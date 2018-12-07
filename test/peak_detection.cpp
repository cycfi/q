/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/low_pass.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/feature_detection.hpp>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;

void process(std::string name, q::frequency cutoff)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
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

   q::wav_writer wav{
      "results/peak_" + name + ".wav", n_channels, sps
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

