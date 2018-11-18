/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q_io/audio_file.hpp>
#include <q/envelope.hpp>
#include <vector>
#include <string>

namespace q = cycfi::q;
namespace audio_file = q::audio_file;
using namespace q::literals;

void process(
   std::string name
 , q::duration attack = 100_ms
 , q::duration decay = 300_ms)
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

   // Envelope processor
   q::envelope_processor::config env_config;
   env_config.attack = attack;
   env_config.decay = decay;

   auto env = q::envelope_processor{ env_config, sps };

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;

      auto s = in[i];

      // Original signal
      out[ch1] = s;

      // Envelope processor
      auto _env = env(s);
      out[ch2] = _env;
      out[ch3] = env.envelope();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
           "results/env_proc_" + name + ".wav", n_channels, sps
   };
   wav.write(out);
}

int main()
{
   process("1-Low E");
   process("Tapping D");
   process("Hammer-Pull High E");
   process("Bend-Slide G");
   process("GStaccato", 10_ms, 50_ms);
   process("GLines1");

   return 0;
}