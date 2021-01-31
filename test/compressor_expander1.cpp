/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/dynamic.hpp>
#include <q_io/audio_file.hpp>
#include <vector>
#include <string>

namespace q = cycfi::q;
using namespace q::literals;

void process(std::string name)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////

   constexpr auto n_channels = 4;
   std::vector<float> out(src.length() * n_channels);

   // Envelope
   auto env = q::peak_envelope_follower{ 10_s, sps };

   // Compressors
   auto comp = q::compressor{ -18_dB, 1.0/4 };
   auto comp2 = q::soft_knee_compressor{ -18_dB, 3_dB, 1.0/4 };
   auto makeup_gain = 4.0f;

   // Expander
   auto exp = q::expander{ -18_dB, 2.0/1.0 };

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;
      auto ch4 = pos+3;

      auto s = in[i];

      // Original signal
      out[ch1] = s;

      // Envelope
      auto env_out = q::decibel(env(std::abs(s)));

      // Compressor
      auto gain = float(comp(env_out)) * makeup_gain;
      out[ch2] = s * gain;

      // Soft Knee Compressor
      gain = float(comp2(env_out)) * makeup_gain;
      out[ch3] = s * gain;

      // Expander
      gain = float(exp(env_out));
      out[ch4] = s * gain;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/comp_exp_" + name + ".wav", n_channels, sps
   );
   wav.write(out);
}

int main()
{
   process("1a-Low-E");
   process("Tapping D");
   process("Hammer-Pull High E");
   process("Bend-Slide G");
   process("GStaccato");

   return 0;
}