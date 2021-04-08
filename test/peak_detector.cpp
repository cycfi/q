/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/lowpass.hpp>
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

   ////////////////////////////////////////////////////////////////////////////
   // Detect waveform peaks

   constexpr auto n_channels = 3;
   std::vector<float> out(src.length() * n_channels);
   auto i = out.begin();

   q::one_pole_lowpass lp{ cutoff, sps };
   q::peak pk{ 0.7f, -60_dB };
   q::peak_envelope_follower env{ cutoff.period() * 5, sps };

   for (auto s : in)
   {
      // Low pass
      s = lp(s);
      *i++ = s;

      *i++ = pk(s, env(s)) * 0.8;
      *i++ = env();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/peak_" + name + ".wav", n_channels, sps
   );
   wav.write(out);
}

int main()
{
   process("1a-Low-E", 329.64_Hz);
   process("1b-Low-E-12th", 329.64_Hz);
   process("2a-A", 440.00_Hz);
   process("2b-A-12th", 440.00_Hz);
   process("3a-D", 587.32_Hz);
   process("3b-D-12th", 587.32_Hz);
   process("4a-G", 784.00_Hz);
   process("4b-G-12th", 784.00_Hz);
   process("5a-B", 987.76_Hz);
   process("5b-B-12th", 987.76_Hz);
   process("6a-High-E", 1318.52_Hz);
   process("6b-High-E-12th", 1318.52_Hz);

   process("Tapping D", 587.32_Hz);
   process("Hammer-Pull High E", 1318.52_Hz);

   return 0;
}

