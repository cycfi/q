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

   constexpr auto n_channels = 4;

   std::vector<float> out(src.length() * n_channels);
   auto i = out.begin();

   q::decibel  comp_threshold = -18_dB;
   q::decibel  comp_width = 3_dB;
   float       comp_slope = 1.0/4;
   float       makeup_gain = 4;

   auto comp = q::soft_knee_compressor{ comp_threshold, comp_width, comp_slope };
   constexpr q::clip clip;

   // Envelope
   auto env = q::fast_envelope_follower{ hold, sps };
   auto env2 = q::peak_envelope_follower{ decay, sps };

   // Attack / Decay
   auto env_shaper = q::envelope_shaper{ 10_ms, decay, 100_ms, -40_dB, sps };

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;
      auto ch3 = pos+2;
      auto ch4 = pos+3;

      // Compressor
      auto _env2 = env2(std::abs(in[i]));
      auto gain = float(comp(_env2)) * makeup_gain;
      auto s = clip(in[i] * gain);

      // Original signal (compressed)
      out[ch1] = s;

      // Envelope (using fast_envelope_follower)
      out[ch2] = env(std::abs(s));

      // Attack / Decay (envelope_shaper)
      out[ch3] = env_shaper(out[ch2]);

      // Envelope (using peak_envelope_follower)
      out[ch4] = _env2;

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
   process("GLines1", g.period());

   return 0;
}