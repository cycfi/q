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
namespace audio_file = q::audio_file;
using namespace q::literals;

void process(std::string name)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   auto src = audio_file::reader{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Attack detection

   constexpr auto n_channels = 3;

   std::vector<float> out(src.length() * n_channels);
   auto i = out.begin();

   // Envelope
   auto env = q::envelope_follower{ 10_ms, 1_s, sps };

   // Compressor
   auto comp = q::compressor{ -18_dB, 1.0/4 };
   auto makeup_gain = 3.0f;

   // Expander
   auto exp = q::expander{ -18_dB, 2.0/1.0 };

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;

      auto s = in[i];

      // Original signal
      out[ch1] = s;

      // Envelope
      q::decibel env_out = env(std::abs(s));

      // Compressor
      auto gain = float(comp(env_out)) * makeup_gain;
      out[ch2] = s * gain;

      // Expander
      gain = float(exp(env_out));
      out[ch3] = s * gain;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
           "results/comp_lim2_" + name + ".wav", n_channels, sps
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