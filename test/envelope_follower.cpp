/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/waveshaper.hpp>
#include <q_io/audio_file.hpp>
#include <vector>
#include <string>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;

void process(std::string name, q::duration period)
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
   auto fast = q::fast_envelope_follower{ period/2, sps };
   auto peak = q::peak_envelope_follower{ 2_s, sps };
   auto env = q::envelope_follower{ 2_ms, 2_s, sps };

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

      // Envelope (using standard envelope_follower)
      out[ch2] = env(std::abs(s));

      // Envelope (using peak_envelope_follower)
      out[ch3] = peak(std::abs(s));

      // Envelope (using fast_envelope_follower)
      out[ch4] = fast(std::abs(s));
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/env_follow_" + name + ".wav", n_channels, sps
   );
   wav.write(out);
}

int main()
{
   using namespace notes;

   process("1a-Low-E", low_e.period());
   process("Tapping D", d.period());
   process("Hammer-Pull High E", high_e.period());
   process("Bend-Slide G", g.period());
   process("GStaccato", g.period());
   process("GLines1", g.period());
   process("GLines2", g.period());
   process("GLines3", g.period());
   process("Transient", g.period());

   return 0;
}