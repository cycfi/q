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

   auto cenv = q::envelope_follower{ 2_ms, 1_s, sps };
   auto comp = q::compressor{ -18_dB, 1.0/8 };
   auto makeup_gain = 3.0f;

   auto _diff1 = q::central_difference{};
   auto _diff2 = q::differentiator{};
   auto _fast_env1 = q::fast_envelope_follower{ n };
   auto _fast_env2 = q::fast_envelope_follower{ n };
   auto _env = q::envelope_follower{ 10_ms, 50_ms, sps };
   auto _cmp = q::schmitt_trigger{ -36_dB };
   auto _pulse = q::pulse{ 25_ms, sps };

   for (auto s : in)
   {
      *i++ = s;

      q::decibel env_out = cenv(std::abs(s));
      auto gain = float(comp(env_out)) * makeup_gain;
      s *= gain;

      // Second derivative (acceleration)
      auto d1 = _diff2(_diff1(s));

      // Fast Envelope Follower
      auto fe1 = _fast_env1(d1) * 4;
      auto fe2 = _fast_env2(-d1) * 4;
      auto fe = fe1 + fe2;

      // Peak detection
      auto e = _env(fe);
      auto prev = _cmp();
      auto cm = _cmp(fe, e);
      auto p = _pulse(prev, cm, [&]{ _cmp.y = 0; });

      *i++ = e;
      *i++ = fe;
      *i++ = p * 0.8;
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


