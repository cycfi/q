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

   constexpr auto n_channels = 4;

   std::vector<float> out(src.length() * n_channels);
   auto i = out.begin();

   q::attack                  attack{ 0.8f, 100_ms, sps };
   q::peak_envelope_follower  env{ 1_s, sps };
   constexpr float            slope = 1.0f/10;
   q::compressor_expander     comp{ 0.5f, slope };
   q::clip                    clip;

   float                      onset_threshold = 0.005;
   float                      release_threshold = 0.001;
   float                      threshold = onset_threshold;

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;
      auto ch4 = pos+3;

      auto s = in[i];

      // Main envelope
      auto e = env(std::abs(s));

      // Compressor and noise gate
      if (e > threshold)
      {
         // Compressor + makeup-gain + hard clip
         s = clip(comp(s, e) * 1.0f/slope);
         threshold = release_threshold;
      }
      else
      {
         s = 0.0f;
         threshold = onset_threshold;
      }

      // Original signal
      out[ch1] = s;

      // attack
      auto o = attack(s);
      out[ch2] = o;

      // The attack envelope
      out[ch3] = attack._env();

      // Lowpassed envelope
      out[ch4] = attack._lp();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
      "results/attack_" + name + ".wav", n_channels, sps
   };
   wav.write(out);
}

int main()
{
   process("sin_440");
   process("1-Low E");
   process("2-Low E 2th");
   process("3-A");
   process("4-A 12th");
   process("5-D");
   process("6-D 12th");
   process("7-G");
   process("8-G 12th");
   process("9-B");
   process("10-B 12th");
   process("11-High E");
   process("12-High E 12th");
   process("Tapping D");
   process("Hammer-Pull High E");
   process("Bend-Slide G");
   process("GStaccato");

   return 0;
}