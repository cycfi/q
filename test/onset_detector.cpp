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
#include <vector>

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

   auto max_val = *std::max_element(in.begin(), in.end(),
      [](auto a, auto b) { return std::abs(a) < std::abs(b); }
   );

   ////////////////////////////////////////////////////////////////////////////
   // Detect waveform peaks

   constexpr auto n_channels = 4;
   std::vector<float> out(src.length() * n_channels);
   auto i = out.begin();

   auto _diff1 = q::central_difference{};
   auto _diff2 = q::differentiator{};
   auto _fast_env1 = q::fast_envelope_follower{ 4_ms, sps };
   auto _fast_env2 = q::fast_envelope_follower{ 4_ms, sps };
   auto _env = q::envelope_follower{ 10_ms, 50_ms, sps };
   auto _cmp = q::schmitt_trigger{ -36_dB };
   auto _pulse = q::pulse{ 15_ms, sps };

   for (auto s : in)
   {
      // Normalize
      s *= 1.0 / max_val;
      *i++ = s;

      // Second derivative (acceleration)
      auto d1 = _diff2(_diff1(s));

      // Fast Envelope Follower
      auto fe1 = _fast_env1(d1) * 10;
      auto fe2 = _fast_env2(-d1) * 10;
      auto fe = fe1 + fe2;

      // Peak detection
      auto e = _env(fe);
      auto prev = _cmp();
      auto cm = _cmp(fe, e);
      auto p = _pulse(prev, cm, [&]{ _cmp.y = 0; });

      *i++ = e;
      *i++ = fe;
      *i++ = cm * 0.8;
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/onset_" + name + ".wav", n_channels, sps
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

