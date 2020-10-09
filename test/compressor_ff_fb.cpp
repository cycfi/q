/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/envelope.hpp>
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
   // Feedforeard vs. Feedback Compressor

   constexpr auto n_channels = 3;
   std::vector<float> out(src.length() * n_channels);

   // Envelopes
   auto ff_env = q::envelope_follower{ 10_ms, 1_s, sps };
   auto fb_env = q::envelope_follower{ 10_ms, 1_s, sps };

   // Compressors
   auto ff_comp = q::compressor{ -18_dB, 1.0 / 4 };
   auto fb_comp = q::compressor{ -18_dB, 1.0 / 4 };
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

      // Feedforward Envelope
      auto ff_env_out = q::decibel(ff_env(std::abs(s)));

      // Feedback Envelope (previous value)
      auto fb_env_out = q::decibel(fb_env());

      // Feedforward Compressor
      auto ff_gain = float(ff_comp(ff_env_out)) * makeup_gain;
      out[ch2] = s * ff_gain;

      // Feedback Compressor
      auto fb_gain = float(fb_comp(fb_env_out)) * makeup_gain;
      out[ch3] = s * fb_gain;

      // Update feedback envelope
      fb_env(std::abs(out[ch3]));
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/comp_ff_fb_" + name + ".wav", n_channels, sps
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