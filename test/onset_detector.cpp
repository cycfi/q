/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q_io/audio_file.hpp>
#include <vector>
#include <string>

namespace q = cycfi::q;
using namespace q::literals;

void process(std::string name)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   auto src = q::wav_reader{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Attack detection

   constexpr auto n_channels = 4;

   std::vector<float> out(src.length() * n_channels);
   auto i = out.begin();

   // Our onset detector
   q::onset_detector onset{ 0.8f, 100_ms, sps };

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;
      auto ch4 = pos+3;

      auto s = in[i];

      // Original signal compressed
      out[ch1] = s;

      // Onset detector
      out[ch2] = onset(s);

      // Onset envelope
      out[ch3] = onset._env();

      // Onset envelope low pass filter
      out[ch4] = onset._lp();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = q::wav_writer{
      "results/onset_" + name + ".wav", n_channels, sps
   };
   wav.write(out);
}

int main()
{
   process("1-Low E");
   process("Tapping D");
   process("Hammer-Pull High E");
   process("Bend-Slide G");
   process("GStaccato");

   return 0;
}