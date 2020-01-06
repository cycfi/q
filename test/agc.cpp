/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/moving_average.hpp>
#include <q/fx/special.hpp>
#include <vector>
#include <string>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;
using namespace notes;

void process(std::string name, q::duration hold)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Automatic Gain Control

   constexpr auto n_channels = 2;
   std::vector<float> out(src.length() * n_channels);

   // Envelope
   auto env = q::fast_envelope_follower{ hold, sps };
   auto envf = q::moving_average<float>{ 16 };

   // AGC
   auto agc = q::agc{ 30_dB };

   // Lookahead
   std::size_t lookahead = float(500_us * sps);
   auto delay = q::nf_delay{ lookahead };

   // Noise reduction
   auto nrf = q::moving_average<float>{ 32 };
   auto xfade = q::crossfade{ -20_dB };

   for (auto i = 0; i != in.size(); ++i)
   {
      auto pos = i * n_channels;
      auto ch1 = pos;
      auto ch2 = pos+1;

      auto s = in[i];

      // Original signal
      out[ch1] = s;

      // Envelope
      q::decibel env_out = envf(env(std::abs(s)));

      // Lookahead Delay
      s = delay(s, lookahead);

      // AGC
      auto gain_db = agc(env_out, -10_dB);
      auto agc_result = s * float(gain_db);

      // Noise Reduction
      auto nr_result = nrf(agc_result);
      out[ch2] = xfade(agc_result, nr_result, env_out);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/agc_" + name + ".wav", n_channels, sps
   );
   wav.write(out);
}

int main()
{
   process("1a-Low-E", low_e.period() * 1.1);
   process("1b-Low-E-12th", low_e.period() * 1.1);
   process("Tapping D", d.period() * 1.1);
   process("Hammer-Pull High E", high_e.period() * 1.1);
   process("Bend-Slide G", g.period() * 1.1);
   process("GStaccato", g.period() * 1.1);

   return 0;
}