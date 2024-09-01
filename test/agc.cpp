/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/moving_average.hpp>
#include <q/fx/level_crossfade.hpp>
#include <q/fx/delay.hpp>
#include <q/fx/lowpass.hpp>

#include <vector>
#include <string>
#include "pitch.hpp"

namespace q = cycfi::q;
using namespace q::literals;
using namespace notes;

void process(std::string name, q::duration hold)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   float const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   // Automatic Gain Control

   constexpr auto n_channels = 4;
   std::vector<float> out(src.length() * n_channels);

   // Envelope
   auto env = q::fast_rms_envelope_follower_db{hold, sps};

   // AGC
   auto agc = q::agc{45_dB};
   auto lp = q::one_pole_lowpass{500_Hz, sps};

   // Noise reduction
   auto nrf = q::moving_average{32};
   auto xfade = q::level_crossfade{-20_dB};
   constexpr auto threshold = lin_float(-80_dB);

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
      auto env_out = env(lp(s));

      // AGC
      auto gain_db = agc(env_out, -10_dB);
      auto agc_result = s * lin_float(gain_db);

      // Noise Reduction
      auto nr_result = nrf(agc_result);
      out[ch2] = xfade(agc_result, nr_result, env_out);

      out[ch3] = lin_float(gain_db) / 100;
      out[ch4] = lin_float(env_out);
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
   process("sin_envelope", a.period() * 1.1);
   process("1a-Low-E", low_e.period() * 1.1);
   process("1b-Low-E-12th", low_e.period() * 1.1);
   process("Tapping D", d.period() * 1.1);
   process("Hammer-Pull High E", high_e.period() * 1.1);
   process("Bend-Slide G", g.period() * 1.1);
   process("GStaccato", g.period() * 1.1);

   return 0;
}