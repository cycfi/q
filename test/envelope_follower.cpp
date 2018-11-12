/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/literals.hpp>
#include <q/sfx.hpp>
#include <q/envelope.hpp>
#include <q_io/audio_file.hpp>
#include <vector>
#include <string>
#include "notes.hpp"

namespace q = cycfi::q;
namespace audio_file = q::audio_file;
using namespace q::literals;

void process(std::string name, q::duration hold, q::duration decay = 5_s)
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
   auto env = q::fast_envelope_follower{ hold, sps };

   // Attack / Decay
   auto att_dcy = q::envelope_attack_decay{ 10_ms, decay, sps };

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
      out[ch2] = env(std::abs(s));

      // Attack / Decay
      out[ch3] = att_dcy(out[ch2]);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   auto wav = audio_file::writer{
           "results/env_follow_" + name + ".wav", n_channels, sps
   };
   wav.write(out);
}

int main()
{
   using namespace notes;

   process("1-Low E", low_e.period());
   process("Tapping D", d.period());
   process("Hammer-Pull High E", high_e.period());
   process("Bend-Slide G", g.period());
   process("GStaccato", g.period(), 10_ms);

   return 0;
}