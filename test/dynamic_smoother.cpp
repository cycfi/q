/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/lowpass.hpp>
#include <vector>
#include <string>
#include "pitch.hpp"

namespace q = cycfi::q;
using namespace q::literals;
using namespace notes;

void process(
   std::string name, std::vector<float> const& in
 , float sps, q::frequency f)
{
   constexpr auto n_channels = 3;
   std::vector<float> out(in.size() * n_channels);

   auto ds = q::dynamic_smoother{f*4, 0.9, sps};
   auto lp = q::one_pole_lowpass{f*4, sps};

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;

      auto s = in[i];

      out[ch1] = s;
      out[ch2] = ds(s);
      out[ch3] = lp(s);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/dynamic_smoother_" + name + ".wav", n_channels, sps
   );
   wav.write(out);
}

void process(std::string name, q::frequency f)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   float const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   process(name, in, sps, f);
}

int main()
{
   process("1a-Low-E", low_e);
   process("1b-Low-E-12th", low_e);
   process("1c-Low-E-24th", low_e);
   process("Tapping D", d);
   process("Hammer-Pull High E", high_e);
   process("Bend-Slide G", g);
   process("GStaccato", g);

   return 0;
}