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

   // Our envelope
   auto env_gen = q::envelope(
      q::envelope::config
      {
         50_ms    // attack rate
       , 70_ms    // decay rate
       , -6_dB    // sustain level
       , 30_s     // sustain rate
       , 3_s      // release rate
      }
    , sps
   );

   // Our envelope_tracker
   q::envelope_tracker env_trk{sps};

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;
      auto ch4 = pos+3;

      auto s = in[i];

      s = env_trk(s, env_gen);

      // Signal processed by envelope tracker
      out[ch1] = s;

      // Onset
      out[ch2] = env_trk._onset();

      // Onset envelope
      out[ch3] = env_trk._onset._env();

      // Generated envelope
      out[ch4] = env_gen();
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
           "results/env_trk_" + name + ".wav", n_channels, sps
   };
   wav.write(out);
}

int main()
{
   process("1-Low E");
//   process("Tapping D");
//   process("Hammer-Pull High E");
//   process("Bend-Slide G");
//   process("GStaccato");

   return 0;
}