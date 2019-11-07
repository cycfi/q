/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/lowpass.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/feature_detection.hpp>
#include <q/fx/special.hpp>
#include <q/fx/dynamic.hpp>
#include <vector>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;
using namespace notes;

void process(
   std::string name, std::vector<float> const& in
 , std::uint32_t sps, std::size_t n)
{
   constexpr auto n_channels = 4;
   std::vector<float> out(in.size() * n_channels);

   ////////////////////////////////////////////////////////////////////////////
   // Detect waveform peaks

   auto i = out.begin();

   auto pre_gain = 4.0f;
   auto makeup_gain = 4.0f;
   auto comp_env = q::envelope_follower{ 2_ms, 1_s, sps };
   auto comp = q::compressor{ -24_dB, 1.0/10 };

   auto _diff1 = q::central_difference{};
   auto _diff2 = q::differentiator{};

   auto _pos_env = q::peak_envelope_follower{50_ms, sps };
   auto _neg_env = q::peak_envelope_follower{50_ms, sps };

   auto _slow_env = q::envelope_follower{10_ms, 50_ms, sps };
   auto _trigger = q::schmitt_trigger{ -36_dB };
   auto _pulse = q::monostable{ 15_ms, sps };
   auto _edge = q::rising_edge{};

   for (auto s : in)
   {
      // Second derivative (acceleration)
      auto diff = _diff2(_diff1(s));

      // Compressor
      diff *= pre_gain;
      q::decibel env_out = comp_env(std::abs(diff));
      auto gain = float(comp(env_out)) * makeup_gain;
      diff *= gain;

      // Peak Envelope Followers
      auto pos_env = _pos_env(diff);
      auto neg_env = _neg_env(-diff);
      auto peak_env = (pos_env + neg_env) / 2;

      // Peak detection
      auto slow_env = _slow_env(peak_env);
      auto trigger = _trigger(peak_env, slow_env);
      auto edge = _edge(trigger);
      auto pulse = _pulse(edge);

      *i++ = s;
      *i++ = diff;
      *i++ = peak_env;
      *i++ = pulse * 0.8;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/onset_" + name + ".wav", n_channels, sps
   );
   wav.write(out);
}

void process(std::string name, q::frequency f)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   auto period = f.period();
   std::size_t n = float(period) * sps;
   process(name, in, sps, n * 1.1);
}

int main()
{
   process("1a-Low-E", low_e);
   process("1b-Low-E-12th", low_e);
   process("Tapping D", d);
   process("Hammer-Pull High E", high_e);
   process("Bend-Slide G", g);
   process("GStaccato", g);

   return 0;
}


